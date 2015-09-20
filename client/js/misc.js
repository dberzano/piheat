/// @file misc.js
/// @author Dario Berzano <dario.berzano@gmail.com>
/// Remote heating control for Raspberry Pi: client scripts

(function($, cfg) {

  var heating_status = {
    'on': '#heating-status-on',
    'off': '#heating-status-off',
    'unknown': '#heating-status-unknown'
  };

  var heating_override = {
    'on': '#heating-override-on',
    'off': '#heating-override-off',
    'none': '#heating-override-none',
    'hm': '#heating-override .heating-override-until'
  };

  var update_status = {
    'updated': '#update-status-updated',
    'error': '#update-status-error',
    'updating': '#update-status-updating',
    'password': '#update-status-password'
  };

  var request_status = {
    'error': '#request-status-error',
    'sent': '#request-status-sent'
  }

  var pages = {
    'password': '#page-password',
    'control': '#page-control',
    'errors': '#page-errors',
    'program': '#page-program'
  };

  var controls = {
    'password': '#control-password',
    'turnon': '#control-turnon',
    'turnoff': '#control-turnoff',
    'reload': '#control-reload',
    'debug': '#control-debug',
    'schedule': '#control-schedule',
    'cancel': '#control-cancel',
  }

  var control_containers = {
    'turnon': '#control-container-turnon',
    'turnoff': '#control-container-turnoff',
    'debug': '#control-container-debug',
    'reload': '#control-container-reload',
    'schedule': '#control-container-schedule',
    'cancel': '#control-container-cancel'
  }

  var inputs = {
    'password': '#input-password'
  }

  var texts = {
    'thingname': '#text-thingname',
    'errors': '#text-errors',
    'errors_title': '#text-errors-title',
    'progs': '#piheat-program-content',
    'prog_title': '#piheat-program .panel-heading'
  }

  var templates = {
    'prog_rmbtn': '#prog-rmbtn-tpl',
    'prog_line': '#piheat-program-template',
    'prog_empty': '#prog-empty'
  }

  /// Current status and expected commands
  var CurrentStatus = {
    when: null,
    password : null,
    debug: false,
    tolerance_ms: 15000,
    expect_cmd_result: false,
    msg_update_expect_cmd_result_s: 7,
    lastcmd_id: null,

    redraw_programs: false,
    new_program: [],

    // The following are part of the status JSON
    name: null,
    msg_expiry_s: null,
    msg_update_s: null,
    status: null,
    program: [],
    override_program: null,
    cmd_id: null,
  };

  var check_config = function() {
    if (cfg === null) {
      return 'JSON configuration variable <b>piheat_config</b> not found';
    }
    else if (!cfg.thingid) {
      return '<b>thingid</b> is mandatory';
    }
    else if (isNaN(parseInt(cfg.default_msg_expiry_s)) ||
      parseInt(cfg.default_msg_expiry_s) < 5) {
        return '<b>default_msg_expiry_s</b> must be set to no less than 5 seconds';
    }
    else if (isNaN(parseInt(cfg.default_msg_update_s)) ||
      parseInt(cfg.default_msg_update_s) < 5) {
        return '<b>default_msg_update_s</b> must be set to no less than 5 seconds';
    }
    return null;
  };

  var init = function() {

    var check_config_result = check_config();
    if (check_config_result != null) {
      // found errors in configuration
      $(texts.errors_title).text('Configuration error');
      $(texts.errors).html( check_config_result );
      $(pages.control).hide();
      $(pages.password).hide();
      $(pages.program).hide();
      $(pages.errors).show();
      return;
    }

    CurrentStatus.msg_expiry_s = cfg.default_msg_expiry_s;
    Logger.log('init', 'initial message expiry set to ' + cfg.default_msg_expiry_s + ' seconds');

    CurrentStatus.msg_update_s = cfg.default_msg_update_s;
    Logger.log('init', 'initial message update set to ' + cfg.default_msg_update_s + ' seconds');

    CurrentStatus.name = cfg.thingid;

    $(pages.control).hide();
    $(pages.password).show();
    $(pages.errors).hide();
    $(pages.program).hide();

    Display.draw_status();

    // actions for turn on/off buttons
    $(controls.turnon).click( Control.turn_on );
    $(controls.turnoff).click( Control.turn_off );
    $(controls.schedule).click( Control.schedule );
    $(controls.cancel).click( Control.cancel );
    $(controls.reload).click( Control.reload );

    // debug button
    if (CurrentStatus.debug) {
      $(controls.debug).click( Control.debug );
      $(control_containers.debug).show();
    }
    else {
      $(control_containers.debug).hide();
    }

    $(controls.password).click(function() {

      // Password button pressed
      CurrentStatus.password = $(inputs.password).val();
      $(pages.control).show();
      $(pages.password).hide();
      $(pages.program).show();

      // Set password "obfuscated" in URL
      window.location.hash = forge.util.encode64(CurrentStatus.password);

      // Reset errors
      Display.request_error(false);
      Display.updated(false, false);
      Display.draw_programs();

      // Commence loops
      Control.read_status_loop();
      Display.last_updated_loop();
      Display.request_received_loop();

    });
    $(inputs.password).keypress(function(evt) {
      // press Enter on password field to submit
      if (evt.which == 13) {
        $(controls.password).trigger('click');
      }
    });
    $(inputs.password).focus();

    // Do we have an hash already? Use it as a password. Give the ability to
    // bookmark it with the password
    if (window.location.hash) {
      $(inputs.password).val( forge.util.decode64(window.location.hash.substring(1)) );
      $(controls.password).trigger('click');
    }

  };

  // ************************************************************************ //
  var Timespan = {

    create : function(begin, end) {
      a = begin.getUTCHours()*100 + begin.getUTCMinutes();
      b = end.getUTCHours()*100 + end.getUTCMinutes();
      return { begin: a, end: b };
    }

  };

  // ************************************************************************ //
  var Logger = {

    pad : function(num, digits) {
      var s = num.toString();
      digits -= s.length;
      for (i=0; i<digits; i++) {
        s = '0' + s;
      }
      return s;
    },

    log : function(func, message) {
      var now = new Date();
      console.log(
        now.getFullYear().toString() +
        Logger.pad(now.getMonth()+1, 2) +
        Logger.pad(now.getDate(), 2) +
        '-' +
        Logger.pad(now.getHours(), 2) +
        Logger.pad(now.getMinutes(), 2) +
        Logger.pad(now.getSeconds(), 2) +
        ' [' + func + '] ' + message
      );
    }

  };

  // ************************************************************************ //
  var Display = {

    last_updated_timeout : null,

    draw_programs : function(drawNew) {

      // note: new_program is in current timezone
      //       program is in UTC

      if (!drawNew) {
        CurrentStatus.new_program = [];
        Logger.log('Control.schedule', 'Converting hours from UTC');
        $.each(CurrentStatus.program, function (key,item) {
          d = new Date();
          d.setUTCHours(parseInt(item.begin/100));
          d.setUTCMinutes(item.begin%100);
          newitem = { begin: d.getHours()*100+d.getMinutes() };
          d.setUTCHours(parseInt(item.end/100));
          d.setUTCMinutes(item.end%100);
          newitem.end = d.getHours()*100+d.getMinutes();
          CurrentStatus.new_program.push(newitem);
        });

      }
      CurrentStatus.new_program.sort(function(a, b) {return a.begin-b.begin});

      Logger.log('Display.draw_programs', 'Drawing programs');

      $(texts.progs).empty();
      $(texts.prog_title).empty();

      tpl = $(templates.prog_line).clone();
      tpl.removeAttr("id");
      tpl.addClass("prog-head");
      tpl.find(".prog-remove").remove();
      tpl.show()
      tpl.appendTo(texts.prog_title)
      tpl.find(".clockpicker").clockpicker();
      tpl.find(".prog-new button").click(function() {
        beg = $(this).closest(".prog-head").find(".prog-begin").val();
        end = $(this).closest(".prog-head").find(".prog-end").val();
        beg = parseInt(beg.replace(":", ""), 10);
        end = parseInt(end.replace(":", ""), 10);
        if (beg > end) {
          // Swap
          beg = beg + end;
          end = beg - end;
          beg = beg - end;
        }
        Logger.log("<click event>", "Clicked: " + beg + "->" + end);
        Logger.log("<click event>", "Before: " + JSON.stringify( CurrentStatus.new_program ));
        exists = CurrentStatus.new_program.find(function (item) {
          return item.begin == beg && item.end == end;
        });
        if (!exists) {
          CurrentStatus.new_program.push({ begin: beg, end: end });
          CurrentStatus.new_program.sort(function(a, b) {return a.begin-b.begin});
          Display.draw_programs(true);
        }
        Logger.log("<click event>", "After: " + JSON.stringify( CurrentStatus.new_program ));
      });

      tb = $("<table></table>").addClass("table").appendTo(texts.progs);
      $.each(CurrentStatus.new_program, function(key, item) {

        rw = $("<tr></tr>").appendTo(tb);

        beg_str = item.begin.toString();
        while (beg_str.length < 4) beg_str = "0"+beg_str;
        beg_str = beg_str.substring(0,2)+":"+beg_str.substring(2,4);
        $("<td>"+beg_str+"</td>").attr("align", "center").appendTo(rw);

        end_str = item.end.toString();
        while (end_str.length < 4) end_str = "0"+end_str;
        end_str = end_str.substring(0,2)+":"+end_str.substring(2,4);
        $("<td>"+end_str+"</td>").attr("align", "center").appendTo(rw);

        Logger.log('Display.draw_programs', 'Program: '+beg_str+'->'+end_str)

        rmb = $(templates.prog_rmbtn).clone();
        rmb.css("display", "");
        td = $("<td></td>").attr("align", "center").appendTo(rw);
        rmb.appendTo(td);
        rmb.data("prog-begin", item.begin);
        rmb.data("prog-end", item.end);
        rmb.click(function() {
          begn = parseInt($(this).data("prog-begin"));
          endn = parseInt($(this).data("prog-end"));
          Logger.log("<click event>", "Removing: " + begn + ", " + endn);
          CurrentStatus.new_program = CurrentStatus.new_program.filter(function (item) {
            return item.begin != begn || item.end != endn;
          });
          Display.draw_programs(true);
          Logger.log("<click event>", "After removal: " + JSON.stringify( CurrentStatus.new_program ));
        });

      });

      if ($(tb).is(":empty")) {
        tb.replaceWith( $(templates.prog_empty).clone().show() );
      }

      CurrentStatus.redraw_programs = false;
    },

    draw_status : function() {
      $(texts.thingname).text(CurrentStatus.name);
      $.each(heating_status, function(key, value) {
        // hide all status labels
        $(value).hide();
      });
      $.each(heating_override, function(key, value) {
        // hide all override status labels
        $(value).hide();
      });
      if (CurrentStatus.status === null) {
        $([control_containers.turnon,
           control_containers.turnoff,
           control_containers.schedule,
           control_containers.cancel].join(",")).hide();
        $(control_containers.reload).show();
        $(heating_status.unknown).show();
      }
      else {
        $([control_containers.turnon,
           control_containers.turnoff,
           control_containers.schedule].join(",")).show();
        $(control_containers.reload).hide();
        $(CurrentStatus.status ? heating_status.on : heating_status.off).show();

        if (CurrentStatus.override_program) {
          $(control_containers.cancel).show();
          $(CurrentStatus.override_program.status ? heating_override.on : heating_override.off)
            .show();

          d = new Date();
          d.setUTCHours(parseInt(CurrentStatus.override_program.end/100));
          d.setUTCMinutes(CurrentStatus.override_program.end%100);
          dstr = d.getMinutes().toString();
          if (dstr.length < 2) dstr = "0"+dstr;
          dstr = d.getHours() + ":" + dstr;

          $(heating_override.hm).text(dstr).show();

          Logger.log('Display.draw_status', 'Override: ' + JSON.stringify(CurrentStatus.override_program) );
          Logger.log('Display.draw_status', 'Override: ' + d );
        }
        else {
          $(control_containers.cancel).hide();
          $(heating_override.none).show();
        }
      }
      if (CurrentStatus.redraw_programs) Display.draw_programs();
    },

    last_updated_loop : function() {
      if (Display.last_updated_timeout) clearTimeout(Display.last_updated_timeout);
      Display.last_updated(CurrentStatus.when);
      Display.last_updated_timeout = setTimeout(Display.last_updated_loop, 5000);
    },

    last_updated : function(when) {
      if (when) {
        var diff = (new Date()-when) / 1000;  // seconds
        if (diff < 30) msg = 'just now';
        else if (diff < 60) msg = 'less than a minute ago';
        else {
          diff /= 60;
          if (diff < 60) {
            msg = Math.round(diff) + ' min ago';
          }
        }
        $(update_status.updated).text(msg);
        $(update_status.updated).show();
      }
      else {
        $(update_status.updated).hide();
      }
    },

    request_received_loop : function() {
      if (Display.request_received_timeout) {
        clearTimeout(Display.request_received_timeout);
      }
      Display.request_received();
      Display.request_received_timeout = setTimeout(Display.request_received_loop, 5000);
    },

    request_received : function() {
      if (CurrentStatus.expect_cmd_result) {
        $(request_status.sent).show();
        $([controls.turnon, controls.turnoff, controls.schedule, controls.cancel,
           texts.progs+" :input",
           texts.prog_title+" :input"].join(",")).prop('disabled', true);
      }
      else {
        $(request_status.sent).hide();
        $([controls.turnon, controls.turnoff, controls.schedule, controls.cancel,
           texts.progs+" :input",
           texts.prog_title+" :input"].join(",")).prop('disabled', false);
      }
      if (CurrentStatus.status === null) {
        $(texts.prog_title+" :input").prop('disabled', true);
      }
    },

    request_error : function(iserr) {
      if (iserr) $(request_status.error).show();
      else $(request_status.error).hide();
    },

    updating : function() {
      $(controls.reload).prop('disabled', true);
      $(update_status.updating).fadeIn( { duration: 'slow', queue: true } );
    },

    updated : function(iserr, ispwderr) {
      $(controls.reload).prop('disabled', false);
      $(update_status.updating).fadeOut( { duration: 'slow', queue: true } );
      if (iserr) {
        $(update_status.error).show();
      }
      else {
        $(update_status.error).hide();
      }
      if (ispwderr) {
        $(update_status.password).show();
      }
      else {
        $(update_status.password).hide();
      }
    }

  };

  // ************************************************************************ //
  var Cipher = {

    encrypt : function(obj) {
      var iv = forge.random.getBytesSync(16);  // raw bytes
      var cleartext = JSON.stringify(obj, null, 0);  // string

      // ensure password is 256 bits long by hashing it
      var md = forge.md.sha256.create();
      md.update(CurrentStatus.password);
      var pwd = md.digest();  // raw bytes -- can use .toHex()

      // encrypt message
      var aes_cbc = forge.cipher.createCipher('AES-CBC', pwd);
      aes_cbc.start({ iv: iv });
      aes_cbc.update( forge.util.createBuffer(Cipher.native2ascii(cleartext)) );
      aes_cbc.finish();

      return {
        "nonce": forge.util.encode64(iv),
        "payload": forge.util.encode64(aes_cbc.output.bytes())  // aes_cbc.output is a buffer
      };

    },

    decrypt : function(obj) {

      var iv, enctext;

      if (obj.nonce) {
        iv = forge.util.decode64(obj.nonce);
      }
      else {
        Logger.log('Cipher.decrypt', 'malformed message: cannot find "nonce"');
        return null;
      }

      if (obj.payload) {
        enctext = forge.util.decode64(obj.payload);
      }
      else {
        Logger.log('Cipher.decrypt', 'malformed message: cannot find "payload"');
        return null;
      }

      // ensure password is 256 bits long by hashing it
      var md = forge.md.sha256.create();
      md.update(CurrentStatus.password);
      var pwd = md.digest();  // raw bytes -- can use .toHex()

      // decrypt message
      var aes_cbc = forge.cipher.createDecipher('AES-CBC', pwd);
      aes_cbc.start({ iv: iv });
      aes_cbc.update( forge.util.createBuffer(enctext, 'raw') );
      aes_cbc.finish();

      // wrong password errors are found during JSON parsing
      var obj;
      try {
        dec = aes_cbc.output.bytes();
        obj = JSON.parse( dec );
      }
      catch (e) {
        Logger.log('Cipher.decrypt', 'data is unreadable (maybe wrong password?): ' + e);
        return null;
      }

      return obj;
    },

    // http://lithium.homenet.org/~shanq/bitsnbytes/native2ascii_en.html
    native2ascii : function(str) {
      var out = '';
      for (var i=0; i<str.length; i++) {
        if (str.charCodeAt(i) < 0x80) {
          out += str.charAt(i);
        }
        else {
          var u = '' + str.charCodeAt(i).toString(16);
          out += '\\u' + (u.length === 2 ? '00' + u : u.length === 3 ? '0' + u : u);
        }
      }
      return out;
    }

  };

  // ************************************************************************ //
  var Control = {

    read_status_timeout : null,

    read_status_loop : function(do_delay) {

      var timeout_ms;

      if (Control.read_status_timeout) {
        clearTimeout(Control.read_status_timeout);
      }

      if (do_delay) {
        // read status after a delay
        Logger.log('Control.read_status_loop', 'delaying read_status by ' +
          CurrentStatus.msg_update_expect_cmd_result_s + ' s');
        Control.read_status_timeout = setTimeout(
          Control.read_status_loop,
          CurrentStatus.msg_update_expect_cmd_result_s*1000
        );
      }
      else {
        Control.read_status();

        if (CurrentStatus.expect_cmd_result) {
          timeout_ms = CurrentStatus.msg_update_expect_cmd_result_s * 1000;
        }
        else {
          timeout_ms = CurrentStatus.msg_update_s * 1000;
        }

        Logger.log('Control.read_status_loop', 'next read_status in ' + timeout_ms + ' ms');
        Control.read_status_timeout = setTimeout( Control.read_status_loop, timeout_ms );
      }

    },

    read_status : function() {

      Display.updating();

      $.get('https://dweet.io/get/dweets/for/' + cfg.thingid)
        .done(function(data) {

          var now = new Date();
          var item_date;
          var password_error = false;
          var NewStatus = null;
          var NewCommand = null;

          // data is an object; most recent is on top, so we can break at first valid entry
          $.each( data.with, function(key, item) {

            item_date = new Date(item.created);

            if ( now - item_date < CurrentStatus.msg_expiry_s*1000 ) {
              // consider only "recent" dweets (can be configured)

              // nonce must be unique
              nonce_raw = item.content.nonce;
              if (typeof nonce_raw === 'undefined') {
                Logger.log('Control.read_status', 'nonce not found for current dweet');
                return true;  // continue from $.each()
              }

              for (j=data.with.length-1; j>key; j--) {
                other_nonce_raw = data.with[j].content.nonce;
                if (nonce_raw == other_nonce_raw) {
                  Logger.log('Control.read_status',
                    'duplicate nonce (' + nonce_raw + '): ignoring, possible replay attack!');
                  return true;  // continue from $.each()
                }
              }

              msg = Cipher.decrypt(item.content);
              if (msg == null) {
                Logger.log('Control.read_status', 'cannot decrypt, ignoring');
                password_error = true;
                return true;  // continue from $.each()
              }

              item_real_date = new Date(msg.timestamp);

              // check timestamp consistency (CurrentStatus.tolerance_ms must be set)
              if ( isNaN(item_real_date.getTime()) ||
                Math.abs(item_date-item_real_date) > CurrentStatus.tolerance_ms ) {
                Logger.log('Control.read_status', 'message/server timestamps mismatch, ignoring');
                return true;
              }

              if (NewStatus === null && msg.type == 'status') {
                try {
                  NewStatus = {};
                  NewStatus.when = item_real_date;
                  NewStatus.status = msg.status;
                  NewStatus.msg_expiry_s = parseInt(msg.msgexp_s);
                  NewStatus.msg_update_s = parseInt(msg.msgupd_s);
                  NewStatus.program = msg.program.sort();
                  NewStatus.override_program = msg.override_program;
                  NewStatus.cmd_id = msg.lastcmd_id;
                  NewStatus.name = msg.name.toString();
                  Logger.log('Control.read_status',
                             'Valid status received: ' + JSON.stringify(msg));
                }
                catch (e) {
                  NewStatus = null;
                  Logger.log('Control.read_status',
                             'Invalid status received (' + e + '), skipping: ' +
                              JSON.stringify(msg));
                }
              }
              else if (NewCommand === null && msg.type == 'command') {
                try {
                  NewCommand = {}
                  NewCommand.cmd_id = msg.id.toString();
                  Logger.log('Control.read_status',
                             'Last command received: ' + JSON.stringify(msg));
                }
                catch (e) {
                  NewCommand = null;
                  Logger.log('Control.read_status',
                             'Invalid last command received (' + e + '), skipping: ' +
                              JSON.stringify(msg));
                }
              }

            }
            else {
              Logger.log('Control.read_status', 'message expired: ignoring the rest');
              return false;  // break from $.each()
            }

          });

          // Check what changed
          if (NewStatus) {
            if (CurrentStatus.cmd_id != NewStatus.cmd_id) {
              CurrentStatus.redraw_programs = true;
            }
            CurrentStatus.status = NewStatus.status;
            CurrentStatus.msg_expiry_s = NewStatus.msg_expiry_s;
            CurrentStatus.msg_update_s = NewStatus.msg_update_s;
            CurrentStatus.program = NewStatus.program;
            CurrentStatus.override_program = NewStatus.override_program;
            CurrentStatus.cmd_id = NewStatus.cmd_id;
            CurrentStatus.name = msg.name;
            CurrentStatus.when = NewStatus.when;
          }
          else {
            // Status is unknown
            Logger.log('Control.read_status', 'Current status is unknown: PiHeat offline?');
            CurrentStatus.status = null;
            CurrentStatus.when = null;
            CurrentStatus.override_program = null;
            CurrentStatus.cmd_id = null;
            if (CurrentStatus.program.length > 0) {
              CurrentStatus.redraw_programs = true;
              CurrentStatus.program = [];
            }
          }

          CurrentStatus.expect_cmd_result = (NewCommand &&
                                             NewCommand.cmd_id != CurrentStatus.cmd_id) ?
                                            true : false;
          Logger.log('Control.read_status',
                     'Expecting new command: '+ CurrentStatus.expect_cmd_result);
          Display.updated(false, password_error);
          Display.draw_status();
          Display.request_received_loop();
          Display.last_updated_loop();

        })
        .fail(function() {
          Display.updated(true, false);
          Logger.log('Control.read_status', 'failed reading dweets');
        })
    },

    turn_on : function() {
      now = new Date();
      later = new Date(now);
      later.setHours(later.getHours()+2)
      ovr = Timespan.create(now, later);
      ovr.status = true;
      CurrentStatus.override_program = ovr;
      Control.req();
    },

    turn_off : function() {
      now = new Date();
      later = new Date(now);
      later.setMinutes(later.getMinutes()-1)
      later.setHours(later.getHours()+24)
      ovr = Timespan.create(now, later);
      ovr.status = false;
      CurrentStatus.override_program = ovr;
      Control.req();
    },

    schedule : function() {
      CurrentStatus.program = CurrentStatus.new_program;
      Logger.log('Control.schedule', 'Converting hours to UTC');
      $.each(CurrentStatus.new_program, function (key,item) {
        d = new Date();
        d.setHours( parseInt(item.begin/100) );
        d.setMinutes( item.begin%100 );
        CurrentStatus.new_program[key].begin = d.getUTCHours()*100+d.getUTCMinutes();
        d.setHours( parseInt(item.end/100) );
        d.setMinutes( item.end%100 );
        CurrentStatus.new_program[key].end = d.getUTCHours()*100+d.getUTCMinutes();
        Logger.log('Control.schedule', '==> '+JSON.stringify(CurrentStatus.new_program[key]));
      });
      Control.req();
    },

    cancel : function() {
      // Cancel the override, does not touch programs
      CurrentStatus.override_program = null;
      Control.req();
    },

    reload : function() {

      $(inputs.password).val('');
      window.location.hash = '';

      $(pages.control).hide();
      $(pages.password).show();
      $(pages.program).hide();

      // end all loops
      clearTimeout(Control.read_status_timeout);
      clearTimeout(Display.last_updated_timeout);
      clearTimeout(Display.request_received_timeout);

      $(inputs.password).focus();

    },

    debug : function() {
      Logger.log('Control.debug', 'this is the debug action');
      Control.read_status();
    },

    req : function() {
      content = { type: 'command',
                  timestamp: (new Date()).toISOString(),
                  program: CurrentStatus.program,
                  override_program: CurrentStatus.override_program,
                  id: forge.util.encode64(forge.random.getBytesSync(30)) }
      Logger.log('Control.req', 'sending request: ' + JSON.stringify(content));
      $.post(
        'https://dweet.io/dweet/for/' + cfg.thingid,
        Cipher.encrypt(content)
      )
        .fail(function() {
          Display.request_error(true);
          Logger.log('Control.req', 'requesting command failed');
        })
        .done(function () {
          CurrentStatus.expect_cmd_result = true;
          Display.request_error(false);
          Display.request_received_loop();
          Control.read_status_loop(true);  // true = delayed request
        });

    }

  };

  // entry point
  $(document).ready(init);

})(jQuery, (typeof piheat_config !== 'undefined') ? piheat_config : null);
