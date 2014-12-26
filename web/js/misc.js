/* misc.js -- by Dario Berzano <dario.berzano@gmail.com>
 */

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

  var encryption_password = '';

  var current_status = {
    status : null,
    when : null,
    new_status : null,
    new_status_when : null
  };

  var init = function() {

    $(pages.control).hide();
    $(pages.password).show();

    $(texts.thingname).text( cfg.thingid );

    Display.heating_status();

    // turn on/off buttons
    $(controls.turnon).click( Control.turn_on );
    $(controls.turnoff).click( Control.turn_off );

    $(controls.password).click(function() {

      // password button pressed
      encryption_password = $(inputs.password).val();
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
      if (evt.which == 13) {
        $(controls.password).trigger('click');
      }
    });
    $(inputs.password).focus();

    // TODO: actually check password (skip it for now)
    $(controls.password).trigger('click');

  };

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

      if (current_status.status == 'on') {
        $(heating_status.on).show();
        $(control_containers.turnoff).show();
      }
      else if (current_status.status == 'off') {
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
      Display.last_updated(current_status.when);
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

      if (current_status.new_status && current_status.new_status != current_status.status) {
        // command not yet accepted
        $(request_status.sent).show();
        var disable_turnon = (current_status.new_status == 'on');
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

    Request : {
      on : 'on',
      off : 'off',
      status : 'status'
    },

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
              console.log('dweet has expired, ignoring all subsequent dweets');
              return false;  // break from $.each()
            }

          });

          // what is our status and last command?
          current_status.status = status;
          current_status.new_status = new_status;
          if (status) {
            console.log('[status] ' + status + ' on ' + status_date);
            current_status.when = status_date;
          }
          else {
            console.log('[status] <unknown>');
            current_status.when = null;
          }
          if (new_status) {
            console.log('[new_status] ' + new_status + ' on ' + new_status_date);
            current_status.new_status_when = new_status_date;
          }
          else {
            console.log('[new_status] <unknown>');
            current_status.new_status_when = null;
          }
          Display.update_error(false);
          Display.heating_status();
          Display.last_updated_loop();
          Display.request_received_loop();

        })
        .fail(function() {
          Display.update_error(true);
          console.log('reading dweets failed');
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

      console.log('requesting command ' + req);

      $.post(
        'https://dweet.io/dweet/for/'+cfg.thingid,
        { type: 'command', command: req }
      )
        .fail(function() {
          Display.request_error(true);
          console.log('posting command failed');
        })
        .done(function () {
          Display.request_error(false);
          if (req == 'turnon') {
            current_status.new_status_when = new Date();
            current_status.new_status = 'on';
          }
          else if (req == 'turnoff') {
            current_status.new_status_when = new Date();
            current_status.new_status = 'off';
          }
          Display.request_received_loop();
        });

    }

  };

  // entry point
  $(document).ready( init );

})(jQuery, piheat_config);
