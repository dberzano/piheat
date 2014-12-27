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
    'control': '#page-control'
  };

  var controls = {
    'password': '#control-password',
    'turnon': '#control-turnon',
    'turnoff': '#control-turnoff'
  }

  var control_containers = {
    'turnon': '#control-container-turnon',
    'turnoff': '#control-container-turnoff'
  }

  var inputs = {
    'password': '#input-password'
  }

  var texts = {
    'thingname': '#text-thingname'
  }

  /// Current status and expected commands
  var CurrentStatus = {
    status : null,
    when : null,
    new_status : null,
    new_status_when : null,
    password : null
  };

  var init = function() {

    $(pages.control).hide();
    $(pages.password).show();

    $(texts.thingname).text( cfg.thingid );

    Display.heating_status();

    // actions for turn on/off buttons
    $(controls.turnon).click( Control.turn_on );
    $(controls.turnoff).click( Control.turn_off );

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

    // TODO: actually check password (skip it for now)
    $(controls.password).trigger('click');

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

  }

  var Display = {

    last_updated_timeout : null,

    heating_status : function() {

      $.each(heating_status, function(key, value) {
        // hide all status labels
        $(value).hide();
      });
      $.each(control_containers, function(key, value) {
        // hide all turn on/off buttons
        $(value).hide();
      })

      if (CurrentStatus.status == 'on') {
        $(heating_status.on).show();
        $(control_containers.turnoff).show();
      }
      else if (CurrentStatus.status == 'off') {
        $(heating_status.off).show();
        $(control_containers.turnon).show();
      }
      else {
        // invalid/unknown
        $(heating_status.unknown).show();
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

  var Control = {

    read_status_timeout : null,

    read_status_loop : function() {
      if (Control.read_status_timeout) {
        clearTimeout(Control.read_status_timeout);
      }
      Control.read_status();
      Control.read_status_timeout = setTimeout(Control.read_status_loop, 10000);
    },

    read_status : function() {

      $.get('https://dweet.io/get/dweets/for/'+cfg.thingid)
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

            if ( now - item_date < cfg.commands_expire_s*1000 ) {
              // consider only "recent" dweets (can be configured)
              if (!status && item.content.type == 'status') {
                // TODO: check if status is valid
                status = item.content.status;
                status_date = item_date;
              }
              else if (!new_status && item.content.type == 'command') {
                command = item.content.command;
                if (command == 'turnon') {
                  new_status = 'on';
                  new_status_date = item_date;
                }
                else if (command == 'turnoff') {
                  new_status = 'off';
                  new_status_date = item_date;
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

    request_status : function() {
      Control.push_request('status');
    },

    push_request : function(req) {

      Logger.log('Control.push_request', 'requesting command: \"' + req + '\"');

      $.post(
        'https://dweet.io/dweet/for/'+cfg.thingid,
        { type: 'command', command: req }
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

})(jQuery, piheat_config);
