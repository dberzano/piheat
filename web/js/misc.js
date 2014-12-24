/* misc.js -- by Dario Berzano <dario.berzano@gmail.com>
 */

(function($) {

  var heating_status = {
    'on': '#heating-status-on',
    'off': '#heating-status-off',
    'unknown': '#heating-status-unknown'
  };

  var pages = {
    'password': '#page-password',
    'control': '#page-control'
  };

  var controls = {
    'password': '#control-password'
  }

  var inputs = {
    'password': '#input-password'
  }

  var encryption_password = '';

  var init = function() {

    $(pages.control).hide();
    $(pages.password).show();

    $(controls.password).click(function () {
      // password button pressed
      encryption_password = $(inputs.password).val();
      $(pages.control).show();
      $(pages.password).hide();
    });

  };

  // entry point
  $(document).ready( init );

})(jQuery);
