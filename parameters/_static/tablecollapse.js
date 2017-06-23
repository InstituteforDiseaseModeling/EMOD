//Wrap the whole mess in the following function to make it work in Drupal
(function($) {

  $(document).ready(function () {

    // Initialize table to fadeOut
    $('.toggle-table').fadeIn();

    //Bind Expand-button method
    $('.collapse-table-button').click(
      function() {
        $('.toggle-table').fadeOut();
      }
    );

    //Bind the toggle button method
    $('.toggle-button').click(
    function() {
      $(this).parent().find("table").fadeToggle( "2000", "linear" );
    });

  });

})(jQuery);