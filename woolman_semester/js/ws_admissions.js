(function ($, D) {

function admissions_org_handler() {

  $('#edit-organization-name-wrapper ~ div').hide();
  $('#edit-organization-name').blur(
    function(){

      if (this.value.match("ID#")) {
        $('#edit-organization-name-wrapper ~ div').hide(600);
        $('.record-message').remove();
        $('#edit-organization-name').after('<span class="record-message" style="color:green; margin-left:12px">existing record</span>');
        }
      else if (this.value==""){
        $('#edit-organization-name-wrapper ~ div').hide(600);
        $('.record-message').remove();
        }
      else {
        $('#edit-organization-name-wrapper ~ div').show(600);
        $('.record-message').remove();
        $('#edit-organization-name').after('<span class="record-message" style="color:red; margin-left:12px">new record</span>');
        }
    }
  );
}

function admissions_contact_handler() {

  $('#edit-contact-name-wrapper ~ div:not(#edit-contact-to-case-wrapper)').hide();
  var conLabel = $('#edit-contact-name-wrapper label').html();
  $('#edit-contact-name').blur(
    function(){
      if (this.value.match("ID#")) {
        $('#edit-contact-name-wrapper ~ div:not(#edit-contact-to-case-wrapper)').hide(600);
        $('.contact-message').remove();
        $('#edit-contact-name').attr('size', '50');
        $('#edit-contact-name-wrapper label').html(conLabel);
        $('#edit-contact-name').after('<span class="contact-message" style="color:green; margin-left:12px">existing person</span>');
        }
      else if (this.value==""){
        $('#edit-contact-name-wrapper ~ div:not(#edit-contact-to-case-wrapper)').hide(600);
        $('.contact-message').remove();
        $('#edit-contact-name').attr('size', '50');
        $('#edit-contact-name-wrapper label').html(conLabel);
        }
      else {
        $('#edit-contact-name-wrapper ~ div:not(#edit-contact-to-case-wrapper)').show(600);
        $('.contact-message').remove();
        $('#edit-contact-name-wrapper label').html("First Name:");
        $('#edit-contact-name').attr('size', '30');
        $('#edit-contact-last').after('<span class="contact-message" style="color:red; margin-left:12px">new person</span>');
        }
    }
  );
}

function outreach_person_handler() {
  $('#edit-outreach-person').blur(
    function(){
      $('.outreach-message').remove();
      if (!this.value.match("ID#")) {
      $('#edit-outreach-person').after('<span class="outreach-message" style="color:red; margin-left:12px">Person not found, please try again.</span>');
      }
      });
}

function homestay_handler() {

  $('#edit-homebody-name-wrapper ~ div').hide();
  var homeLabel = $('#edit-homebody-name-wrapper .description').html();
  $('#edit-homebody-name').blur(
    function(){

      if (this.value.match("ID#")) {
        $('#edit-homebody-name-wrapper ~ div').hide(600);
        $('.homestay-message').remove();
        $('#edit-homebody-name-wrapper .description').html(homeLabel);
        $('#edit-homebody-name').after('<span class="homestay-message" style="color:green; margin-left:12px">existing person</span>');
        }
      else if (this.value==""){
        $('#edit-homebody-name-wrapper ~ div').hide(600);
        $('#edit-homebody-name-wrapper .description').html(homeLabel);
        $('.homestay-message').remove();
        }
      else {
        $('#edit-homebody-name-wrapper ~ div').show(600);
        $('.homestay-message').remove();
        $('#edit-homebody-name-wrapper .description').html("First Name");
        $('#edit-homebody-last').after('<span class="homestay-message" style="color:red; margin-left:12px">new person</span>');
        }
    }
  );
}

function activity_handler() {
  $('#edit-create-activity').click(
    function(){
      $("#edit-create-activity-wrapper .description").html("Automatically create activity record for this trip");
      $('#edit-activity-notes').removeAttr("disabled");

      if ($('#edit-create-activity').attr("checked")==false) {
        $("#edit-create-activity-wrapper .description").html('Don\'t create activity record. <span style="color:red;">Not recommended unless using this form to bulk-import non outreach trip contacts.</span>');
        $('#edit-activity-notes').attr("disabled","disabled");
      }
    });
}

  $(document).ready(function () {

    $("fieldset:odd").addClass('odd');
    
    $('#edit-organization-website').focus(function(){
      if($(this).val() == '') {
        $(this).val('http://');
      }
    });
    $('#edit-organization-website').blur(function(){
      if($(this).val() == 'http://') {
        $(this).val('');
      }
    });

    admissions_org_handler();

    admissions_contact_handler();

    outreach_person_handler();

    activity_handler();

    homestay_handler();

  });

}(jQuery, Drupal));
