/* misc.js -- by Dario Berzano <dario.berzano@gmail.com>
 */

(function($, cfg) {

  var heating_status = {
    'on': '#heating-status-on',
    'off': '#heating-status-off',
    'unknown': '#heating-status-unknown'
  };

  var update_status = {
    'updating': '#update-status-updating',
    'updated': '#update-status-updated',
    'failure': '#update-status-failure',
    'sent': '#update-status-sent'
  };

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

      // heating status is unknown
      $(heating_status.on).hide();
      $(heating_status.off).hide();
      $(heating_status.unknown).show();
      $(control_containers.turnoff).hide();
      $(control_containers.turnon).hide();

      // perform initial query
      Control.push_request( Control.Request.status );

    });

  };

  var Control = {

    Request : {
      on : 'on',
      off : 'off',
      status : 'status'
    },

    read_status : function() {
    },

    push_request : function(req) {

      $.each( update_status, function(key, value) {
        if (key == 'sent') $(value).show();
        else $(value).hide();
      });

      $.post(
        'https://dweet.io/dweet/for/'+cfg.thingid,
        { command: req }
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
