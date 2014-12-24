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
    'error': '#request-status-error'
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

  var init = function() {

    $(pages.control).hide();
    $(pages.password).show();

    $(texts.thingname).text( cfg.thingid );

    $(controls.password).click(function() {

      // password button pressed
      encryption_password = $(inputs.password).val();
      $(pages.control).show();
      $(pages.password).hide();

      // test read status
      Control.read_status();

      // commence loop
      Control.read_status_loop();

    });

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
          var status = null;

          // data is an object; most recent is on top, so we can break at first valid entry
          $.each( data.with, function(key, item) {

            if ( now - new Date(item.created) < cfg.commands_expire_s*1000 ) {
              // consider only "recent" dweets (can be configured)
              if (item.content.type == 'status') {
                $.each(Request, function(key, value) {
                  if (item.content.status == value) {
                    status = key;
                  }
                });
                console.log( 'status found: '+item.content.status );
              }
              else {
                console.log('not a status dweet, ignoring... content follows');
                console.log(item.content);
              }
            }
            else {
              console.log('dweet has expired, ignoring all subsequent dweets');
              return false;  // break from $.each()
            }

          });

        })
        .fail(function() {
          console.log('reading dweets failed')
        })
    },

    push_request : function(req) {

      $.each( update_status, function(key, value) {
        if (key == 'sent') $(value).show();
        else $(value).hide();
      });

      $.post(
        'https://dweet.io/dweet/for/'+cfg.thingid,
        { type: 'command', command: req }
      )
        .done( function() {
          $.each( update_status, function(key, value) {
            if (key == 'updating') $(value).show();
            else $(value).hide();
          });
        })
        .fail( function() {
          $.each( update_status, function(key, value) {
            if (key == 'failure') $(value).show();
            else $(value).hide();
          });
        });

    }

  };

  // entry point
  $(document).ready( init );

})(jQuery, piheat_config);
