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
    'updated': '#update-status-updated'
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
      $(update_status.updating).hide();
      $(update_status.updated).hide();
      $(control_containers.turnoff).hide();
      $(control_containers.turnon).hide();

      // perform initial query
      Control.push_request( Control.Request.status );

    });

  };

  var Control = {

    Request : {
      on : 1,
      off : 2,
      status : 3
    },

    read_status : function() {
    },

    push_request : function(req) {
    }

  };

  // entry point
  $(document).ready( init );

})(jQuery, piheat_config);
