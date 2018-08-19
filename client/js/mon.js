(function ($) {

  // Globals.
  var table_max = 15;
  var chart_max = 2500;
  var bin_timespan = 10*60*1000;  // bin data within this interval
  var oldest_min = 24*60*60*1000;  // minimum value for the oldest parameter
  var outdated = 10*60*1000;  // data older than this is outdated
  var oldest_tol = 10*60*1000;  // tolerance for oldest date
  var vmin = 2.7;  // below this voltage reading, the sensor does not work (flat battery)
  var vmax = 3.7;  // maximum voltage reading at full charge
  var battery_level_labels = [ "0", "25", "50", "75", "100" ];  // class labels to battery levels
  var battery_plug_label = "plug";  // class label to power plug indicator

  // https://flatuicolors.com/
  var flat = { "nephritis":   "#27ae60",    // green
               "pumpkin":     "#d35400",    // orange
               "pomegranate": "#c0392b",    // red
               "wisteria":    "#8e44ad",    // violet
               "clouds":      "#ecf0f1",    // gray (light)
               "peter_river": "#3498db" };  // azure

  // Debug messages. Supports sprintf format.
  var debug = function(msg) {
    console.log((new Date()).toISOString() + " DEBUG:mon: " +
                sprintf.apply(null, Array.prototype.slice.call(arguments)));
  };

  // Checks if obj contains all the given properties (array) that must be valid
  // and defined. A Boolean is returned.
  var isvalid = function(obj, vals) {
    var valid = true;
    $.each(vals, function(index, val) {
      if (!(val in obj)) {
        valid = false;
      }
      else if (Object.prototype.toString.call(obj[val]) == "[object Date]") {
        if (isNaN(obj[val].getTime())) valid = false;
      }
      else if (Object.prototype.toString.call(obj[val]) == "[object Number]") {
        if (isNaN(obj[val])) valid = false;
      }
      if (!valid) return false;
    });
    return valid;
  };

  // Return as UTC string the current date plus the given offset in millisecs.
  var nowoffset = function(offset) {
    return (new Date(Date.now() + offset)).toISOString();
  };

  var shardsuffix = function(date) {
    return sprintf("/%04d/%02d/%02d.json",
                   date.getUTCFullYear(), date.getUTCMonth()+1, date.getUTCDate());
  };

  // Sensors
  var SensorDisplay = function(name, url, woeid) {

    var entries = [];
    var chart_temp = null;
    var chart_humi = null;
    var chart_volt = null;
    var sel_timestamp = null;
    var gauge_temp = null;
    var gauge_humi = null;
    var gauge_volt = null;
    var data_temp = null;
    var data_humi = null;
    var data_volt = null;
    var evt_temp = null;
    var evt_humi = null;
    var evt_volt = null;
    var dom_id = null;
    var shard_date = new Date();
    var timeout = 45000;
    var timeout_id = null;
    var weather_timestamp = 0;
    var weather_every = 1800000;  // 30 min
    var loading = false;
    var oldest = oldest_min;

    var get = function() {
      if (timeout_id == null) { timeout_id = setTimeout(get, 0); return; }

      // Update Yahoo! Weather info if appropriate
      if (woeid > -1 && (new Date()).getTime()-weather_timestamp > weather_every) {
        yahoo_weather_url = "https://query.yahooapis.com/v1/public/yql?q=select%20item.condition%20from%20weather.forecast%20where%20woeid%20%3D%20" + woeid + "&format=json&env=store%3A%2F%2Fdatatables.org%2Falltableswithkeys";
        $.get(yahoo_weather_url)
          .done(function(data) {
            yahoo_weather_code = undefined;
            temp_f = undefined;
            try {
              yahoo_weather_code = data.query.results.channel.item.condition.code;
              temp_f = data.query.results.channel.item.condition.temp;
              if (yahoo_weather_code === undefined || temp_f === undefined) {
                throw "";
              }
            }
            catch (e) {
              debug("%s: could not load Yahoo! Weather information for WOEID %d", name, woeid);
              return;
            }
            temp_c = Math.round(10. * (temp_f - 32.) / 1.8) / 10;
            $("#weather_"+dom_id).html(setWeatherIcon(yahoo_weather_code) + " " + temp_c + "°C");
            weather_timestamp = new Date();
          });
      }

      // Activates blinking of the loading sign
      if (!loading) {
        $("#data_loading_"+dom_id)
          .bind("fade-cycle", function() {
            $(this).fadeOut("slow", function() {
              $(this).fadeIn("slow", function() {
                $(this).trigger("fade-cycle");
              });
            });
          })
          .trigger("fade-cycle");
        loading = true;
      }

      shard_path = shardsuffix(shard_date)
      unique_suffix = "?now="+Date.now()+"&random="+Math.floor(Math.random()*1000000000)
      debug("%s: loading shard %s as %s%s%s", name, shard_path, url, shard_path, unique_suffix)
      $.get(url+shard_path+unique_suffix)
        .always(function(data) {
          // First parameter is xhr on failure, data on success

          if (data.status == 404) {
            // Data might be served statically: 404 is empty, not an error
            debug("%s: interpreting 404 as empty response", name)
            data = [];
          }
          else if (!isNaN(data.status) && data.status != 200) {
            // Actual error
            debug("%s: could not load data this time from shard %s (%d), we will retry in 5 s",
                  name, shard_path, data.status);
            timeout = 5000;
          }

          if (data.constructor === Array) {
            // Success
            debug("%s: ok, shard %s has %d entries", name, shard_path, data.length);
            var new_entries = [];
            $.each(data, function(index, val) {
              var entry = { "timestamp": new Date(val.timestamp),
                            "temp": parseFloat(val.temp),
                            "humi": parseInt(val.humi),
                            "volt": "volt" in val ? parseFloat(val.volt) : -1 };
              if (!isvalid(entry, ["timestamp", "temp", "humi", "volt"])) return true;
              new_entries.push(entry);
            });

            oldest_ts = new Date().getTime()-oldest;
            count_new = 0;
            for (i=0; i<new_entries.length; i++) {
              a = new_entries[i].timestamp.getTime();
              if (a <= oldest_ts-oldest_tol) continue;
              skip = false;
              for (j=0; j<entries.length; j++) {
                b = entries[j].timestamp.getTime();
                if (a == b) { skip = true; break; }
                else if (a > b) { break; }
              }
              if (!skip) { entries.splice(j, 0, new_entries[i]); count_new++; }
            }
            count_rm = 0;
            for (i=entries.length-1; i>=0; i--) {
              a = entries[i].timestamp.getTime();
              if (a <= oldest_ts-oldest_tol) { entries.pop(); count_rm++; }
            }
            debug("%s: we have %d entries (+%d, -%d)", name, entries.length, count_new, count_rm);

            if (entries.length > 0 && new_entries.length > 0 &&
                new_entries[new_entries.length-1].timestamp.getTime() > oldest_ts &&
                entries[entries.length-1].timestamp.getTime() > oldest_ts) {
              shard_date.setUTCDate(shard_date.getUTCDate()-1);
              debug("%s: we need more data", name);
              timeout = 0;
            }
            else {
              // Deactivates blinking of the loading sign
              loading = false;
              $("#data_loading_"+dom_id).unbind("fade-cycle");
              shard_date = new Date();
              debug("%s: loading done", name)
              timeout = 45000;
            }
          }

          // Always
          chart();
          timeout_id = setTimeout(get, timeout);
        });
    };

    var chart = function() {
      var temp_format = new google.visualization.NumberFormat({ "fractionDigits": 1, "suffix": "°C" });
      var humi_format = new google.visualization.NumberFormat({ "fractionDigits": 0, "suffix": "%" });
      var volt_format = new google.visualization.NumberFormat({ "fractionDigits": 2, "suffix": "V" });
      data_temp = new google.visualization.DataTable();
      data_temp.addColumn("datetime", "Date/time");
      data_temp.addColumn({ "type": "string", "role": "annotation" });
      data_temp.addColumn("number", "Temperature [°C]");
      data_humi = new google.visualization.DataTable();
      data_humi.addColumn("datetime", "Date/time");
      data_humi.addColumn({ "type": "string", "role": "annotation" });
      data_humi.addColumn("number", "Relative humidity [%]");
      data_volt = new google.visualization.DataTable();
      data_volt.addColumn("datetime", "Date/time");
      data_volt.addColumn({ "type": "string", "role": "annotation" });
      data_volt.addColumn("number", "Input tension [V]");

      // Rebin (keep rightmost timestamp per bin)
      var rebinned = [];
      var rebin_next = entries.length ? $.extend({ "count": 1 }, entries[0]) : null;
      var push_rebinned = function() {
        if (!rebin_next) return;
        rebin_next.temp /= rebin_next.count;
        rebin_next.humi /= rebin_next.count;
        rebin_next.volt /= rebin_next.count;
        delete rebin_next.count;
        rebinned.push(rebin_next);
        rebin_next = null;
      };
      for (i=0; i<entries.length; i++) {
        if (rebin_next.timestamp-entries[i].timestamp > bin_timespan) {
          push_rebinned();
          rebin_next = $.extend({ "count": 1 }, entries[i]);
        }
        else {
          rebin_next.temp += entries[i].temp;
          rebin_next.humi += entries[i].humi;
          rebin_next.volt += entries[i].volt;
          rebin_next.count++;
        }
      }
      push_rebinned();
      debug("%s: entries before/after rebin: %d/%d", name, entries.length, rebinned.length);

      // Take at most chart_max entries
      for (i=Math.min(rebinned.length, chart_max)-1; i>=0; i--) {
        data_temp.addRow([ rebinned[i].timestamp, null, rebinned[i].temp ]);
        data_humi.addRow([ rebinned[i].timestamp, null, rebinned[i].humi ]);
        data_volt.addRow([ rebinned[i].timestamp, null, rebinned[i].volt ]);
      }

      var now = new Date();
      data_temp.addRow([ now, "this is now", null ]);
      data_humi.addRow([ now, "this is now", null ]);
      data_volt.addRow([ now, "this is now", null ]);

      var sel_all = data_temp.getFilteredRows([{ "column": 0,
                                                 "minValue": new Date(sel_timestamp-bin_timespan),
                                                 "maxValue": new Date(sel_timestamp-(-bin_timespan)) }]);
      var sel_ref = null;
      $.each(sel_all, function(index, val) {
        var ts = data_temp.getValue(val, 0);
        var delta = Math.abs(ts-sel_timestamp);
        if (!sel_ref || delta<sel_ref.delta) {
          sel_ref = { "row": val,
                      "timestamp": ts,
                      "delta": delta };
        }
      });
      sel_ref = sel_ref !== null && data_temp.getValue(sel_ref.row, 2) !== null ?
                [{ "row": sel_ref.row, "column": 2 }] : null;

      temp_format.format(data_temp, 2);
      humi_format.format(data_humi, 2);
      volt_format.format(data_volt, 2);
      var opts = {
        "hAxis": { "viewWindow": { "max": new Date(now-(0.05*(data_temp.getValue(0, 0)-now))),
                                   "min": data_temp.getValue(0, 0) },
                   "textStyle": { "color": flat.peter_river },
                   "gridlines": { "count": -1,
                                  "color": flat.peter_river,
                                  "units": { "days": { "format": [ "EEE dd" ] },
                                             "hours": { "format": [ "H:mm", "H" ] } } },
                   "minorGridlines": { "color": flat.clouds,
                                       "units": { "hours": { "format": [ "H:mm", "ha" ]},
                                                  "minutes": { "format": [ "H:mm", ":mm"] } } } },
        "legend": { "position": "none" },
        "fontName": "Montserrat",
        "fontSize": 14,
        "height": 250,
        "annotations": { "style": "line" },
        "tooltip": { "trigger": "selection" },
        "chartArea": { "top": 10, "left": 100, "height": 180, "width": "100%" }
      };

      // Draw temp chart
      chart_temp = chart_temp ? chart_temp :
                                new google.visualization.AreaChart(document.getElementById("chart_temp_"+dom_id));
      chart_temp.draw(data_temp, $.extend({ "colors": [ flat.nephritis ],
                                            "vAxis": { "title": "Temperature [°C]",
                                                       "textStyle": { "color": flat.nephritis } } }, opts));
      if (evt_temp) google.visualization.events.removeListener(evt_temp);
      if (sel_ref) chart_temp.setSelection(sel_ref);
      evt_temp = google.visualization.events.addListener(chart_temp, "select",
        function() {
          var sel = chart_temp.getSelection();
          chart_humi.setSelection(sel);
          chart_volt.setSelection(sel);
          sel_timestamp = sel.length && "row" in sel[0] ? data_temp.getValue(sel[0].row, 0) : null;
        });

      // Draw humi chart
      chart_humi = chart_humi ? chart_humi :
                                new google.visualization.AreaChart(document.getElementById("chart_humi_"+dom_id));
      chart_humi.draw(data_humi, $.extend({ "colors": [ flat.wisteria ],
                                            "vAxis": { "title": "Relative humidity [%]",
                                                       "textStyle": { "color": flat.wisteria } } }, opts));
      if (evt_humi) google.visualization.events.removeListener(evt_humi);
      if (sel_ref) chart_humi.setSelection(sel_ref);
      evt_humi = google.visualization.events.addListener(chart_humi, "select",
        function() {
          var sel = chart_humi.getSelection();
          chart_temp.setSelection(sel);
          chart_volt.setSelection(sel);
          sel_timestamp = sel.length && "row" in sel[0] ? data_humi.getValue(sel[0].row, 0) : null;
        });

      // Draw volt chart
      chart_volt = chart_volt ? chart_volt :
                                new google.visualization.AreaChart(document.getElementById("chart_volt_"+dom_id));
      chart_volt.draw(data_volt, $.extend({ "colors": [ flat.pumpkin ],
                                            "vAxis": { "title": "Input tension [V]",
                                                       "textStyle": { "color": flat.pumpkin } } }, opts));
      if (evt_volt) google.visualization.events.removeListener(evt_volt);
      if (sel_ref) chart_volt.setSelection(sel_ref);
      evt_volt = google.visualization.events.addListener(chart_volt, "select",
        function() {
          var sel = chart_volt.getSelection();
          chart_temp.setSelection(sel);
          chart_humi.setSelection(sel);
          sel_timestamp = sel.length && "row" in sel[0] ? data_volt.getValue(sel[0].row, 0) : null;
        });

      gauge_temp = gauge_temp ? gauge_temp :
                                new google.visualization.Gauge(document.getElementById("gauge_temp_"+dom_id));
      gauge_humi = gauge_humi ? gauge_humi :
                                new google.visualization.Gauge(document.getElementById("gauge_humi_"+dom_id));
      gauge_volt = gauge_volt ? gauge_volt :
                                new google.visualization.Gauge(document.getElementById("gauge_volt_"+dom_id));
      var gauge_temp_data = new google.visualization.DataTable();
      var gauge_humi_data = new google.visualization.DataTable();
      var gauge_volt_data = new google.visualization.DataTable();
      var gauge_opts = {};
      $.each([ gauge_temp_data, gauge_humi_data, gauge_volt_data ], function(i, g) {
        g.addColumn("string", "Label");
        g.addColumn("number", "Value");
        if (!entries.length) g.addRow([ "Wait...", -10 ]);
        else if (Date.now()-entries[0].timestamp>outdated) g.addRow([ "n/a", -10 ]);
      });
      if (!gauge_temp_data.getNumberOfRows()) {
        gauge_temp_data.addRow([ "Temp", entries[0].temp ]);
        gauge_humi_data.addRow([ "Humi", entries[0].humi ]);
        gauge_volt_data.addRow([ "Volt", entries[0].volt ]);
      }
      temp_format.format(gauge_temp_data, 1);
      gauge_temp.draw(gauge_temp_data,
        $.extend({ "min":        12, "max":      34,
                   "yellowFrom": 12, "yellowTo": 19, "yellowColor": flat.peter_river,
                   "greenFrom":  19, "greenTo":  25, "greenColor":  flat.nephritis,
                   "redFrom":    25, "redTo":    34, "redColor":    flat.pumpkin,
                   "width":     150, "height":  150, "minorTicks": 5 }, gauge_opts));
      humi_format.format(gauge_humi_data, 1);
      gauge_humi.draw(gauge_humi_data,
        $.extend({ "width": 150,
                   "height": 150,
                   "minorTicks": 5 }, gauge_opts));
      volt_format.format(gauge_volt_data, 1);
      gauge_volt.draw(gauge_volt_data,
        $.extend({ "min":        2.0, "max":      4.0,
                   "greenFrom":  3.1, "greenTo":  4.0, "greenColor":  flat.nephritis,
                   "yellowFrom": 2.7, "yellowTo": 3.1, "yellowColor": flat.peter_river,
                   "redFrom":    2.0, "redTo":    2.7, "redColor":    flat.pumpkin,
                   "width":      150, "height":   150, "minorTicks": 5 }, gauge_opts));

      // Fill time label
      last_date = moment(entries.length ? entries[entries.length-1].timestamp : new Date());
      $("#time_label_"+dom_id).text( last_date.calendar().toLowerCase() );

      // Update battery level
      batt_label = battery_plug_label;
      if (entries.length && Date.now()-entries[0].timestamp > outdated) {
        batt_label = "0"
      }
      else if (entries.length && entries[0].volt >= 0) {
        vnorm = Math.max(Math.min((entries[0].volt-vmin)/(vmax-vmin), 1.), 0.);
        batt_label = battery_level_labels[ Math.round(vnorm*(battery_level_labels.length-1)) ];
      }

      $("#battery_level_"+dom_id).children().hide();
      $("#battery_level_"+dom_id+" .level_"+batt_label).show();
    };

    var dom = function() {
      dom_id = parseInt(Math.random()*100000);
      var dom_obj = $("#sensor_model")
                      .clone()
                      .attr("id", "sensor_"+dom_id)
                      .show();
      classes = [ "chart_temp", "chart_humi", "chart_volt",
                  "gauge_temp", "gauge_humi", "gauge_volt",
                  "data_loading",
                  "battery_level",
                  "weather",
                  "time_label",
                  "time_plus1d", "time_minus1d",
                  "time_plus1w", "time_minus1w",
                  "time_plus1m", "time_minus1m" ];
      $.each(classes,
             function(index, label) {
               dom_obj
                 .find("."+label)
                 .attr("id", label+"_"+dom_id)
                 .filter(function() { return label.match(/^time_(plus|minus)/); } )
                 .click(function() {
                   a = label.match(/time_(plus|minus)1([dwm])/);
                   if (loading && a[1] == "plus") {
                     debug("%s: not adding data while still loading");
                     return;
                   }
                   if      (a[2] == "d") tdiff = 24*60*60*1000;
                   else if (a[2] == "w") tdiff = 7*24*60*60*1000;
                   else if (a[2] == "m") tdiff = 30*24*60*60*1000;
                   tdiff *= a[1] == "minus" ? -1 : 1;
                   oldest += tdiff;
                   if (oldest < oldest_min) oldest = oldest_min;
                   shard_date = new Date();
                   clearTimeout(timeout_id);
                   timeout_id = setTimeout(get, 0);
                   debug("%s: rescale series of %d ms in the %s", name, Math.abs(tdiff), tdiff >= 0 ? "past" : "future");
                 })
                 .end()
                 .filter(function() { return label == "battery_level"; } )
                 .click(function() {
                   $("#chart_volt_"+dom_id+",#gauge_volt_"+dom_id).toggle();
                   chart();
                 })
                 .end()
                 .filter(function() { return (label.match(/^(gauge|chart)_volt$/)); })
                 .hide()
                 .end();
             });
      var head = dom_obj.find("h2 .title").first();
      head.text(name);
      dom_obj.children().appendTo("#sensors_container");
    };

    dom();
    chart();
    get();
    $(window).resize($.throttle(500, chart));
  };

  // Entry point.
  var main = function() {
    mon_conf = mon_conf ? mon_conf : [];
    $.each(mon_conf, function(index, entry) {
      SensorDisplay(entry.name, entry.url, entry.woeid);
    });
  };

  google.charts.load("current", { "packages": ["corechart", "gauge"] });
  google.charts.setOnLoadCallback(main);

})(jQuery);
