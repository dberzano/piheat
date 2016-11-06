(function ($) {

  // Globals.
  var table_max = 15;
  var chart_max = 2500;
  var bin_timespan = 10*60*1000;  // bin data within this interval
  var oldest = 24*60*60*1000;  // oldest data to request from server
  var outdated = 5*60*1000;  // data older than this is outdated
  var oldest_tol = 10*60*1000;  // tolerance for oldest date

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

  // Sensors
  var SensorDisplay = function(name, url) {

    var entries = [];
    var lock_chart = false;
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
    var page = 1;
    var eof = false;
    var timeout = 45000;

    var get = function() {
      $.get(url+"?page="+page)
        .done(function(data) {
          debug("%s: ok, page %d has %d entries", name, page, data.length);
          var new_entries = [];
          $.each(data, function(index, val) {
            var entry = { "timestamp": new Date(val.timestamp),
                          "temp": parseFloat(val.temp),
                          "humi": parseInt(val.humi),
                          "volt": "volt" in val ? parseFloat(val.volt) : -1 };
            if (!isvalid(entry, ["timestamp", "temp", "humi", "volt"])) return true;
            new_entries.push(entry);
          });

          if (!new_entries.length) eof = true; // no more data before this page
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

          if (entries.length > 0 && !eof &&
              entries[entries.length-1].timestamp.getTime() > oldest_ts) {
            debug("%s: we need more data: next page will be %d", name, page+1);
            page++;
            timeout = 0;
          }
          else {
            page = 1;
            timeout = 45000;
          }
        })
        .fail(function() {
          debug("%s: could not load data this time from page %d, we'll retry in 5 s", name, page);
          timeout = 5000;
        })
        .always(function() {
          chart();
          setTimeout(get, timeout);
        });
    };

    var chart = function() {
      if (lock_chart) return;
      lock_chart = true;
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

      lock_chart = false;
    };

    var dom = function() {
      dom_id = parseInt(Math.random()*100000);
      var dom_obj = $("#sensor_model")
                      .clone()
                      .attr("id", "sensor_"+dom_id)
                      .show();
      dom_obj.find(".chart_temp").attr("id", "chart_temp_"+dom_id);
      dom_obj.find(".chart_humi").attr("id", "chart_humi_"+dom_id);
      dom_obj.find(".chart_volt").attr("id", "chart_volt_"+dom_id);
      dom_obj.find(".gauge_temp").attr("id", "gauge_temp_"+dom_id);
      dom_obj.find(".gauge_humi").attr("id", "gauge_humi_"+dom_id);
      dom_obj.find(".gauge_volt").attr("id", "gauge_volt_"+dom_id);
      var head = dom_obj.find("h2").first();
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
      SensorDisplay(entry.name, entry.url);
    });
  };

  google.charts.load("current", { "packages": ["corechart", "gauge"] });
  google.charts.setOnLoadCallback(main);

})(jQuery);
