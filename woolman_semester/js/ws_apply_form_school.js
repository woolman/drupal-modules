(function ($, D) {

  function change_school_type(speed) {
    if( $("#edit-school-type").val() == "other") {
      $("#school-options, #school-details, .teaches-at .form-item:first-child").hide(speed);
      $("#school-other").show(speed);
      $("#edit-teacher1-employer-other, #edit-teacher2-employer-other").removeAttr( "disabled" );
      $("div[id$=employer-other-wrapper] label").html('Teaches at:');
    }
    else{
      $("#school-options, .teaches-at .form-item:first-child").show(speed);
      $("#school-other").hide(speed);
      $("div[id$=employer-other-wrapper] label").html("&nbsp;");
      if( $("input:radio[name=teacher1_relationship]:checked").val()=="school" ) {
        $("#edit-teacher1-employer-other").attr("disabled", "disabled");
      }
      if( $("input:radio[name=teacher2_relationship]:checked").val()=="school" ) {
        $("#edit-teacher2-employer-other").attr("disabled", "disabled");
      }
      if( $("#edit-school-choice").val()=="none" ) {
        $("#school-details").show(speed);
      }
    }
  }

  D.behaviors.ws_school = function(context) {
    $('select[name="school_choice"]', context).not('.ready').addClass('ready').change(function() {
      if ($(this).val() == "none") {
        $("#school-details").show();
      }
      else{
        $("#school-details").hide();
      }
    }).change();
  };

  $(document).ready(function () {
    var cancel = false;

    preventEnterSubmit('#-ws-apply-form-type-school');

    $("#edit-save, #edit-cancel").hover(
      function () { cancel = true; },
      function () { cancel = false;}
    );
    $("#edit-save, #edit-cancel").focus(function () {cancel = true;} );
    $("#edit-save, #edit-cancel").blur(function () {cancel = false;} );

    $("form#-ws-apply-form-type-school").submit(function() {
      if( cancel==true ) {
        return true;
      }else{
        var teacher1 = $("#edit-teacher1-first").val() + " " + $("#edit-teacher1-last").val() + ": " + $("#edit-teacher1-email").val();
        var teacher2 = $("#edit-teacher2-first").val() + " " + $("#edit-teacher2-last").val() + ": " + $("#edit-teacher2-email").val();
        return confirm("Click OK to submit this form.\nTeacher recommendation forms will be sent to:\n"+teacher1+" and "+teacher2+".\nAre these names and addresses correct?");
      }
    });

    if( $("input:radio[name=teacher1_relationship]:checked").val()!="other" ) {
      $("#edit-teacher1-employer-other").attr("disabled", "disabled");
    }

    if( $("input:radio[name=teacher2_relationship]:checked").val()!="other" ) {
      $("#edit-teacher2-employer-other").attr("disabled", "disabled");
    }

    change_school_type(null);

    $("#edit-school-type").change(function() {
      change_school_type(600);
    });

    $("#edit-school-website").focus(function() {
     if ($(this).val() == '') {
       $(this).val('http://');
     }
    });
    $("#edit-school-website").blur(function() {
     if ($(this).val() == 'http://') {
       $(this).val('');
     }
    }).blur();

    $("input:radio[name=teacher1_relationship]").change(function() {
      if( $("input:radio[name=teacher1_relationship]:checked").val()=="school" ) {
        $("#edit-teacher1-employer-other").attr("disabled", "disabled");
      }
      else{
        $("#edit-teacher1-employer-other").removeAttr( "disabled" );
        $("#edit-teacher1-employer-other").focus();
      }
    });

    $("input:radio[name=teacher2_relationship]").change(function() {
      if( $("input:radio[name=teacher2_relationship]:checked").val()=="school" ) {
        $("#edit-teacher2-employer-other").attr("disabled", "disabled");
      }
      else{
        $("#edit-teacher2-employer-other").removeAttr( "disabled" );
        $("#edit-teacher2-employer-other").focus();
      }
    });

  });

}(jQuery, Drupal));
