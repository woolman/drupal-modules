(function ($, D) {

  $(document).ready(function () {

    preventEnterSubmit('#-ws-apply-form-type-form');

    var cancel = false;

    $("#edit-save, #edit-cancel").hover(
      function () { cancel = true; },
      function () { cancel = false;}
    );
    $("#edit-save, #edit-cancel").focus(function () {cancel = true;} );
    $("#edit-save, #edit-cancel").blur(function () {cancel = false;} );

    $("form#-ws-apply-form-type-form").submit(function(){

      if( cancel==true ){
        return true;
      }else{
        var parent1 = $("#edit-parent1-first").val() + " " + $("#edit-parent1-last").val() + ": " + $("#edit-parent1-email").val();
        return confirm("Click OK to submit this application form.\nA questionnaire will be sent to "+parent1+".\nIs this name and address correct?");
      }
    });


   $("#edit-parent1-same").change(function(){
      if ($(this).attr("checked")==true){
        $("#sameaddress1 ~ div").fadeOut(600);
      }
      else{
        $("#sameaddress1 ~ div").fadeIn(600);
      }
    });

   $("#edit-parent2-same").change(function(){
      if ($(this).attr("checked")==true){
        $("#sameaddress2 ~ div").fadeOut(600);
      }
      else{
        $("#sameaddress2 ~ div").fadeIn(600);
      }
    });

    $("#edit-parent1-first").blur(function(){
      if ($(this).val()){
        $(".p1-name").html($(this).val());
      }
      else{
        $(".p1-name").html("Primary Caregiver");
      }
    });

    if ($("#edit-parent1-same").attr("checked")==true){
      $("#sameaddress1 ~ div").hide();
    }
    if ($("#edit-parent2-same").attr("checked")==true){
      $("#sameaddress2 ~ div").hide();
    }
    $("#edit-parent1-first").blur();
  });

}(jQuery, Drupal));
