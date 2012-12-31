$(document).ready(function () {

  $("#woolman-camp-registration-form > div > ul").tabs();
  $("#woolman-camp-registration-form > div > ul").removeClass('ui-corner-top').addClass('ui-corner-left');
  tally();

  $("input:checkbox").change(function() {
    if($(this).attr("checked")==true) {
      $(this).parents('.child-wrapper').find('.questions-wrapper').show(500);
    }else {
      $(this).parents('.register-button').removeClass('selected');
    }
    tally();
  });
  preventEnterSubmit('#woolman-camp-registration-form');
});

function tally() {
  var total = 0;
  $("#tab-nav li a").each(function() {
    var cid = $(this).attr("href");
    var sessions = 0;
    if($(cid+' input:checkbox').length != 0) {
      if($(cid+' input:checkbox:checked').length != 0) {
        total += 1;
      }
      $(cid+' input:checkbox:checked').each(function() {
        sessions += 1;
        $(this).parents('.register-button').addClass('selected');
      });
      if(sessions == 0) {
        $(this).find('.registered-for').html('<span class="unselected-text">Click to Register</span><span class="selected-text">Choose sessions &gt;&gt;</span>');
        $(cid+' .questions-wrapper').hide();
      }else if(sessions == 1) {
        $(this).find('.registered-for').html('Registered for: <strong>1</strong> session');
      }else {
        $(this).find('.registered-for').html('Registered for: <strong>'+sessions+'</strong> sessions');
      }
    }
  });
  if(total == 0) {
    var msg = 'No campers registered';
    $('#edit-submit').attr('disabled','disabled');
  }else if(total == 1) {
    var msg = "You're registering <strong>1</strong> camper";
    $('#edit-submit').removeAttr('disabled');
  }else {
    var msg = "You're registering <strong>"+total+"</strong> campers";
    $('#edit-submit').removeAttr('disabled');
  }
  $('#total').html(msg);
}
