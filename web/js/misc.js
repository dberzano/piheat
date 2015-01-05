/// @file misc.js
/// @author Dario Berzano <dario.berzano@gmail.com>
/// Remote heating control for Raspberry Pi: client scripts

(function($, cfg) {

  var heating_status = {
    'on': '#heating-status-on',
    'off': '#heating-status-off',
    'unknown': '#heating-status-unknown'
  };

  var update_status = {
    'updated': '#update-status-updated',
    'error': '#update-status-error'
  };

  var request_status = {
    'error': '#request-status-error',
    'sent': '#request-status-sent'
  }

  var pages = {
    'password': '#page-password',
    'control': '#page-control',
    'errors': '#page-errors'
  };

  var controls = {
    'password': '#control-password',
    'turnon': '#control-turnon',
    'turnoff': '#control-turnoff'
  }

  var control_containers = {
    'turnon': '#control-container-turnon',
    'turnoff': '#control-container-turnoff',
    'debug': '#control-container-debug'
  }

  var inputs = {
    'password': '#input-password'
  }

  var texts = {
    'thingname': '#text-thingname',
    'errors': '#text-errors',
    'errors_title': '#text-errors-title'
  }

  /// Current status and expected commands
  var CurrentStatus = {
    status : null,
    when : null,
    new_status : null,
    new_status_when : null,
    password : null,
    debug: false,
    msg_expiry_s: null,
    msg_update_s: null,
    name: null,
    tolerance_ms: 15000,
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

    Display.heating_status();

    // actions for turn on/off buttons
    $(controls.turnon).click( Control.turn_on );
    $(controls.turnoff).click( Control.turn_off );

    // debug button
    if (CurrentStatus.debug) {
      $(controls.debug).click( Control.debug );
      $(control_containers.debug).show();
    }
    else {
      $(control_containers.debug).hide();
    }

    $(controls.password).click(function() {

      // password button pressed
      CurrentStatus.password = $(inputs.password).val();
      $(pages.control).show();
      $(pages.password).hide();

      // reset errors
      Display.request_error(false);
      Display.update_error(false);

      // commence loops
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

  };

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

  var Display = {

    last_updated_timeout : null,

    heating_status : function() {

      $.each(heating_status, function(key, value) {
        // hide all status labels
        $(value).hide();
      });

      if (CurrentStatus.status == 'on') {
        $(heating_status.on).show();
        $(control_containers.turnon).hide();
        $(control_containers.turnoff).show();
      }
      else if (CurrentStatus.status == 'off') {
        $(heating_status.off).show();
        $(control_containers.turnon).show();
        $(control_containers.turnoff).hide();
      }
      else {
        // invalid/unknown
        $(heating_status.unknown).show();
        $(control_containers.turnon).hide();
        $(control_containers.turnoff).hide();
      }

      // thing's friendly name
      if ( $(texts.thingname).text() != CurrentStatus.name ) {
        $(texts.thingname).text( CurrentStatus.name );
      }

    },

    last_updated_loop : function() {
      if (Display.last_updated_timeout) {
        clearTimeout(Display.last_updated_timeout);
      }
      Display.last_updated(CurrentStatus.when);
      Display.last_updated_timeout = setTimeout(Display.last_updated_loop, 5000);
    },

    last_updated : function(when) {
      if (when) {
        var diff = (new Date()-when) / 1000;  // seconds
        var msg;
        if (diff < 30) {
          msg = 'just now';
        }
        else if (diff < 60) {
          msg = 'less than a minute ago';
        }
        else {
          diff /= 60;
          if (diff < 60) {
            msg = Math.round(diff) + ' min ago';
          }
        }
        if ( $(update_status.updated).text() != msg ) {
          $(update_status.updated).text(msg);
        }
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

      if (CurrentStatus.new_status && CurrentStatus.new_status != CurrentStatus.status) {
        // command not yet accepted
        $(request_status.sent).show();
        var disable_turnon = (CurrentStatus.new_status == 'on');
        $(controls.turnon).prop('disabled', disable_turnon);
        $(controls.turnoff).prop('disabled', !disable_turnon);
      }
      else {
        $(request_status.sent).hide();
        $(controls.turnon).prop('disabled', false);
        $(controls.turnoff).prop('disabled', false);
      }

    },

    request_error : function(iserr) {
      if (iserr) {
        $(request_status.error).show();
      }
      else {
        $(request_status.error).hide();
      }
    },

    update_error : function(iserr) {
      if (iserr) {
        $(update_status.error).show();
      }
      else {
        $(update_status.error).hide();
      }
    }

  };

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

  var Control = {

    read_status_timeout : null,

    read_status_loop : function() {
      if (Control.read_status_timeout) {
        clearTimeout(Control.read_status_timeout);
      }
      Control.read_status();
      Control.read_status_timeout = setTimeout(
        Control.read_status_loop,
        CurrentStatus.msg_update_s*1000
      );
    },

    read_status : function() {

      $.get('https://dweet.io/get/dweets/for/' + cfg.thingid)
        .done(function(data) {

          var now = new Date();
          var item_date;
          var status = null;
          var new_status = null;
          var status_date = null;
          var new_status_date = null;

          // data is an object; most recent is on top, so we can break at first valid entry
          $.each( data.with, function(key, item) {

            item_date = new Date(item.created);

            if ( now - item_date < CurrentStatus.msg_expiry_s*1000 ) {
              // consider only "recent" dweets (can be configured)

              msg = Cipher.decrypt(item.content);
              if (msg == null) {
                Logger.log('Control.read_status', 'cannot decrypt, ignoring');
                return true;  // continue from $.each()
              }

              item_real_date = new Date(msg.timestamp);

              // check timestamp consistency (CurrentStatus.tolerance_ms must be set)
              if ( isNaN(item_real_date.getTime()) ||
                Math.abs(item_date-item_real_date) > CurrentStatus.tolerance_ms ) {
                Logger.log('Control.read_status', 'message/server timestamps mismatch, ignoring');
                return true;
              }

              if (!status && msg.type == 'status' && typeof msg.status !== 'undefined') {
                status = msg.status.toLowerCase();
                if (status == 'on' || status == 'off') {
                  status_date = item_real_date;

                  try {
                    if (msg.msgexp_s != CurrentStatus.msg_expiry_s && msg.msgexp_s > 5) {
                      CurrentStatus.msg_expiry_s = parseInt(msg.msgexp_s);
                      Logger.log('Control.read_status', 'new message expiry: '+msg.msgexp_s+' s');
                    }
                  }
                  catch (e) {}

                  try {
                    if (msg.msgupd_s != CurrentStatus.msg_update_s && msg.msgupd_s > 5) {
                      CurrentStatus.msg_update_s = parseInt(msg.msgupd_s);
                      Logger.log('Control.read_status', 'new update rate: '+msg.msgupd_s+' s');
                    }
                  }
                  catch (e) {}

                  try {
                    if (msg.name != CurrentStatus.name) {
                      CurrentStatus.name = msg.name;
                      Logger.log('Control.read_status', 'new name: '+msg.name);
                    }
                  }
                  catch (e) {}

                }
                else {
                  // status message is not valid: skip
                  Logger.log('Control.read_status', 'invalid status ignored: \"' + status + '\"');
                  status = null;
                }
              }
              else if (!new_status && msg.type == 'command' && typeof msg.command !== 'undefined') {
                command = msg.command.toLowerCase();
                if (command == 'turnon') {
                  new_status = 'on';
                  new_status_date = item_real_date;
                }
                else if (command == 'turnoff') {
                  new_status = 'off';
                  new_status_date = item_real_date;
                }
                else {
                  // command is not valid: skip
                  Logger.log('Control.read_status', 'invalid command ignored: \"' + command + '\"');
                }
              }
            }
            else {
              Logger.log('Control.read_status', 'message expired: ignoring the rest');
              return false;  // break from $.each()
            }

          });

          // what is our status and last command?
          CurrentStatus.status = status;
          CurrentStatus.new_status = new_status;
          if (status) {
            Logger.log('Control.read_status', 'status: ' + status + ' on ' + status_date);
            CurrentStatus.when = status_date;
          }
          else {
            Logger.log('Control.read_status', 'status: <unknown>');
            CurrentStatus.when = null;
          }
          if (new_status) {
            Logger.log('Control.read_status', 'new_status: ' + new_status + ' on ' + new_status_date);
            CurrentStatus.new_status_when = new_status_date;
          }
          else {
            Logger.log('Control.read_status', 'new_status: <unknown>');
            CurrentStatus.new_status_when = null;
          }
          Display.update_error(false);
          Display.heating_status();
          Display.last_updated_loop();
          Display.request_received_loop();

        })
        .fail(function() {
          Display.update_error(true);
          Logger.log('Control.read_status', 'failed reading dweets');
        })
    },

    turn_on : function() {
      $(controls.turnon).prop('disabled', true);
      Control.push_request('turnon');
    },

    turn_off : function() {
      $(controls.turnoff).prop('disabled', true);
      Control.push_request('turnoff', controls.turnoff);
    },

    debug : function() {
      console.log('no action associated to the debug button');
    },

    request_status : function() {
      Control.push_request('status');
    },

    push_request : function(req) {

      Logger.log('Control.push_request', 'requesting command: \"' + req + '\"');

      $.post(
        'https://dweet.io/dweet/for/' + cfg.thingid,
        Cipher.encrypt( { type: 'command', command: req } )
      )
        .fail(function() {
          Display.request_error(true);
          Logger.log('Control.push_request', 'requesting command failed');
        })
        .done(function () {
          Display.request_error(false);
          if (req == 'turnon') {
            CurrentStatus.new_status_when = new Date();
            CurrentStatus.new_status = 'on';
          }
          else if (req == 'turnoff') {
            CurrentStatus.new_status_when = new Date();
            CurrentStatus.new_status = 'off';
          }
          Display.request_received_loop();
        });

    }

  };

  // entry point
  $(document).ready( init );

})(jQuery, (typeof piheat_config !== 'undefined') ? piheat_config : null);
