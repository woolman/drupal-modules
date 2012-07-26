(function ($, D) {
  $(document).ready(function() {
    $('.datepicker').each(function() {
      var dd = $(this).attr('data-default');
      if (dd) {
        var d = dd.split('-');
        var defaultDate = new Date(d[0], d[1], 1);
        $(this).datepicker({ defaultDate: defaultDate });
        $(this).parents('tr').hide();
      }
      else {
        $(this).datepicker();
      }
    });
    $('#semester-dates tbody').append('<tr><td colspan="3" style="text-align:right;"><a href="#" id="next-semester">+ Add Semester</a></td></tr>');
    $('#next-semester').click(function() {
      $('tr:hidden:first', '#semester-dates').css('display', 'table-row');
      if ($('tr:hidden', '#semester-dates').length < 1) {
        $('#next-semester').parents('tr').hide();
      }
      return false;
    });
  });
}(jQuery, Drupal));
