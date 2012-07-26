(function ($, D) {

  function goGoGadgetPopup(subject, results, richText){
    $("#confirmation-inner").html(results);

    $("#view-confirmation").dialog({
      title: subject,
      modal: true,
      width : "680px",
      height: "560px",
      resizable: false,
      bgiframe: false,
      overlay: {
          opacity: 0.5,
          background: "black"
      },

      beforeclose: function(event, ui) {
          if( richText ){
                $("#wysiwyg-toggle-edit-essay").click();}
          $(this).dialog("destroy");
      },

      open:function() {
          $("#view-confirmation").show();
      },

      buttons: {
          "Submit": function() {
               $("#confirmation-inner").html('<div class="loading"></div>');
               $("form[id^=-ws-apply-form-type]")[0].submit();

            },
          "Not Yet": function() {
              $(this).dialog("close");
          }
        }
      });
  }

  $(document).ready(function() {

    var cancel = false;

    $("#edit-save").hover(
      function () { cancel = true; },
      function () { cancel = false;}
    );
    $("#edit-save").focus(function () {cancel = true;} );
    $("#edit-save").blur(function () {cancel = false;} );

    $("form#-ws-apply-form-type-questions").submit(function(){

      if( cancel==true ){
        return true;
      }else{
        var answer;
        var results = "<p><i>Click the submit button to complete this form. The following answers will be submitted.</i></p>";
        $("form#-ws-apply-form-type-questions .form-item").each(function(){
          results = results+'<h3 class="underline">'+$(this).find("label").html()+"</h3><p>";
          answer = $(this).find("textarea").val();
          if( answer.length < 5 ){
            alert("Please answer all the questions before submitting");
            return false
          }
          results = results+answer.replace(/(\n|\r)/g, "<br />")+"</p>";
          }
        );
        if( answer.length >= 5 ){
          goGoGadgetPopup("Confirm Your Answers", results, false);
        }

        return false;
        }
      });

    $("form#-ws-apply-form-type-essay").submit(function(){
      var richText = false;
      if( $("#wysiwyg-toggle-edit-essay").text() == "Disable rich-text" ){
        $("#wysiwyg-toggle-edit-essay").click();
        richText = true;
      }
      if( cancel==true ){
        return true;
      }else{
        var good = true;
        var results = "<p><i>Click the submit button to complete this form. The following essay will be submitted.</i></p>";
        if( $("form#-ws-apply-form-type-essay .essay-choices").length ){
          var essay = $(".essay-choices .form-radio:checked").parent().text();
          if( !essay ){
            if( richText ) {
              $("#wysiwyg-toggle-edit-essay").click();
            }
            good = false;
            alert("Please select the essay question you are answering.");
          }
        }else{
          essay = $("form#-ws-apply-form-type-essay").siblings("h5").text();
        }
        var body = $("form#-ws-apply-form-type-essay textarea").val();
        if( body.length < 30 && good == true ){
          if( richText ){
            $("#wysiwyg-toggle-edit-essay").click();}
          good = false;
          alert("Please complete the essay before submitting.");
        }
        results = results + '<h3 class="underline">' + essay + '</h3><p>' + body.replace(/(\n|\r)/g, "<br />")+"</p>";

        if( good ){
          goGoGadgetPopup("Finished With Your Essay?", results, richText);
        }

        return false;
        }
      });
  });

}(jQuery, Drupal));
