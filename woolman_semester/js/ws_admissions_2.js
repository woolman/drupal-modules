(function ($, D) {

function create_fieldset(stuff){
  if (admissions_count < 99){
    admissions_count = admissions_count + 1;
    if (!(admissions_count%2)) stuff.addClass("odd");
    stuff.css("display", "none");

    stuff.find("div").each(function () {
      if (this.id){
        this.id = this.id.replace('01-wrapper','0')+admissions_count+'-wrapper';
      }
    });

    stuff.find("input").each(function () {
        this.id = this.id.replace('01','0')+admissions_count;
        this.name = this.name.replace('01','0')+admissions_count;
    });

    stuff.find("textarea").each(function () {
      if (this.id){
        this.id = this.id.replace('01','0')+admissions_count;
        this.name = this.name.replace('01','0')+admissions_count;
      }
    });

    stuff.find("select").each(function () {
      if (this.id){
        this.id = this.id.replace('01','0')+admissions_count;
        this.name = this.name.replace('01','0')+admissions_count;
      }
    });

    stuff.contents("legend").replaceWith('<legend>Student '+admissions_count+'</legend>');

    $("fieldset:last").after(stuff);

    $("fieldset:last").show(600);

    if (admissions_count == 2) $("#killfield").show(600);
  }
  else {
  alert("Sorry, can't do more than 99 at once.");
  }
}

function destroy_fieldset(){
  if (admissions_count > 1) {
    admissions_count = admissions_count - 1;
    if (admissions_count == 1) $("#killfield").hide();
      $("fieldset:last").hide(600, function(){
        $("fieldset:last").remove();
      });
  }
}


function echeck(str) {

    var at="@"
    var dot="."
    var lat=str.indexOf(at)
    var lstr=str.length
    var ldot=str.indexOf(dot)

    if (str.indexOf(at)==-1){
       return false
    }
    if (str.indexOf(at)==-1 || str.indexOf(at)==0 || str.indexOf(at)==lstr){
       return false
    }
    if (str.indexOf(dot)==-1 || str.indexOf(dot)==0 || str.indexOf(dot)==lstr){
        return false
    }
     if (str.indexOf(at,(lat+1))!=-1){
        return false
     }
     if (str.substring(lat-1,lat)==dot || str.substring(lat+1,lat+2)==dot){
        return false
     }
     if (str.indexOf(dot,(lat+2))==-1){
        return false
     }
     if (str.indexOf(" ")!=-1){
        return false
     }
     return true
  }


function form_submission_handler(){
// Prevents accidental form submission by asking for a confirmaiton when form is submitted without clicking submit button
// Verifies email addresses

    var ok = false;

    $("#edit-submit").hover(
      function(){ ok = true; },
      function(){ ok = false; }
    );

    $("#ws-admissions-form").submit(function() {
      if ( ok == true ) {
        var agree = true;
      }
      else {
     var agree = confirm("Are you sure you are done with this form?");
       }
      if (agree==false){ return false;}

      var admissions_val = true;

      $("input[id*='edit-email-student']").each(function(){
        var email = $(this).val();
        $(this).removeClass("error");
        if (email != "" && echeck(email) == false) {
          $(this).addClass("error");
          if (admissions_val == true ){
            var num = $(this).attr("id").replace("edit-email-student-0","");
            alert("Invalid email address for student "+num);
          }
          admissions_val = false;
        }
      });

      if( $("#edit-first-student-01").val() == "" || $("#edit-first-student-01").val() == "" ){
        admissions_val = false;
        alert("Please enter at least one student");
      }

      return admissions_val;
    });
}

  $(document).ready(function () {

    admissions_count = $("fieldset").size();
    admissions_newfieldset = $("fieldset:first").clone(true);

    $("fieldset:last").after('<button type=button id="killfield">[-] One Less</button><button type=button id="addfield">[+] One More</button>');

    $("#addfield").click(function(){
      var stuff;
      stuff = admissions_newfieldset.clone(true);
      create_fieldset(stuff);
    });

    $("#killfield").click(function(){destroy_fieldset()} );

    form_submission_handler();

  });

}(jQuery, Drupal));
