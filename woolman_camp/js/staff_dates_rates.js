(function ($, D) {

  $(document).ready(function() {
    $('.container-inline div[id$="year-wrapper"]').css('display','none');

    $('.number-of-events').change(function() {
      var n = $(this).val();
      var max = $('option:last', this).attr('value');
      if (n < max) {
        $(this).parent().parent().children('.session-dates, .shuttle-times').slice(n).hide(400);
      }
      if (n > 0) {
        $(this).parent().parent().children('.session-dates, .shuttle-times').slice(0, n).show(400);
      }
    });

    $('.number-of-events').each(function() {
      var max = $('option:last', this).attr('value');
      var n = $(this).val();
      if (n < max) {
        $(this).parent().parent().children('.session-dates, .shuttle-times').slice(n).css('display','none');
      }
    });

  });

}(jQuery, Drupal));
