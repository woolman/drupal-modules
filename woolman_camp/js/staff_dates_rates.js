(function ($, D) {

  $(document).ready(function() {
    $('.container-inline div[id$="year-wrapper"]').css('display','none');

    $('.number-of-sessions').change(function() {
      var n = $(this).val();
      if(n<6) {
        $(this).parents('fieldset.session-selection').children('.session-dates').slice(n).hide(400);
      }
      if(n>0) {
        $(this).parents('fieldset.session-selection').children('.session-dates').slice(0,n).show(400);
      }
    });

    $('.number-of-sessions').each(function() {
      var n = $(this).val();
      if(n<6) {
        $(this).parents('fieldset.session-selection').children('.session-dates').slice(n).css('display','none');
      }
    });
    $('.number-of-shuttles').change(function() {
      var n = $(this).val();
      if(n<9) {
        $(this).parents('fieldset.shuttle-selection').children('.shuttle-times').slice(n).hide(400);
      }
      if(n>0) {
        $(this).parents('fieldset.shuttle-selection').children('.shuttle-times').slice(0,n).show(400);
      }
    });

    $('.number-of-shuttles').each(function() {
      var n = $(this).val();
      if(n<9) {
        $(this).parents('fieldset.shuttle-selection').children('.shuttle-times').slice(n).css('display','none');
      }
    });
  });

}(jQuery, Drupal));
