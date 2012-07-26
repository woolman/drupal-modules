function viewActivity( activityID, contactID ) {

    $("#activity-content").html('<div class="loading"></div>');

    $("#view-activity").dialog({
        title: "View Activity",
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
            $(this).dialog("destroy");
        },

        open:function() {
            $("#view-activity").show( );
            $("#activity-content").load( "/civicrm/case/activity/view?snippet=4&cid="+contactID + "&aid=" + activityID);
        },

        buttons: {
            "Done": function() {
                $(this).dialog("close");
                $(this).dialog("destroy");
            }
        }
    });
}

var cj = jQuery; //supports civi's revision functions

// parser to order name field by last name (requires span in-between the first & last names)
$.tablesorter.addParser({
    id: 'lastname',
    is: function(s) {
        return false;
    },
    format: function(s) {
        nam = s.toLowerCase().split('<i></i>');
        return nam[2]+nam[0];
    },
    type: 'text'
});

// parser to order by initial count
$.tablesorter.addParser({
    id: 'countme',
    is: function(s) {
        return false;
    },
    format: function(s) {
        return s.substr(0,2);
    },
    type: 'numeric'
});

function countBoxes(){
  var checks = $("#prospectives input.form-checkbox:checked").length;
  if( checks ){
    $("th.checkbox span").html(checks);
    $("#ws-admissions-prospective-form #edit-submit-1").removeAttr("disabled");
  }else{
    $("th.checkbox span").html("&nbsp;");
    $("#ws-admissions-prospective-form #edit-submit-1").attr("disabled", "disabled");
  }
}

$(document).ready(function() {
  preventEnterSubmit('#ws-admissions-prospective-form');
  var subject = $("#edit-subject").val();
  var details = $("#edit-details").val();

  $("#edit-actions").change(function() {
    $("#edit-actions-wrapper ~ div:not(#make-changes-wrapper)").hide();
    $("#edit-subject-wrapper .description").hide();
    $("#edit-subject, #edit-details").val("");
    $("#make-changes-wrapper input").attr("value","Make Changes");

    if( $(this).val()=="status" ){
      $("#edit-subject-wrapper label").html("Details:");
      $("#edit-subject-wrapper .description").html("Why are you making this change?").show();
      $("#edit-change-status-wrapper, #edit-subject-wrapper").show();
    }
    if( $(this).val()=="rep" ){
      $("#edit-change-rep-wrapper").show();
    }
    if( $(this).val()=="grad" ){
      $("#edit-grad-wrapper").show();
    }
    if( $(this).val()=="sem" ){
      $("#edit-grad-wrapper+div").show();
    }
    if( $(this).val()=="followup" ){
      $("#edit-date-wrapper").show();
      $("#make-changes-wrapper input").attr("value","Schedule");
    }
    if( $(this).val()=="email" ){
      $("#edit-details-wrapper label").html("Message:");
      $("#edit-subject").val(subject);
      $("#edit-details").val(details);
      $("#edit-subject-wrapper label").html("Subject:");
      $("#edit-details-wrapper, #edit-subject-wrapper").show();
      $("#message-tokens").css("display", "inline-block");
      $("#make-changes-wrapper input").attr("value","Send Message");
    }
    if( $(this).val()=="left-msg" || $(this).val()=='act-31' ){
      if( $(this).val()=="left-msg" ){
        $("#edit-subject-wrapper .description").html("(Optional: What were you calling about?)");
      }else{
        $("#edit-subject-wrapper .description").html("(Optional: Details about this FB exchange)");
      }
      $("#edit-subject-wrapper label").html("Details:");
      $("#edit-subject-wrapper .description").show();
      $("#make-changes-wrapper input").attr("value","Record Activity");
      $("#edit-date-wrapper, #edit-time-wrapper, #edit-subject-wrapper").show();
    }
    if( $(this).val()=='act-14' || $(this).val()=='act-28' || $(this).val()=='act-29' ){
      $("#edit-subject-wrapper label").html("Subject:");
      $("#edit-details-wrapper label").html("Details:");
      $("#make-changes-wrapper input").attr("value","Record Activity");
      $("#edit-date-wrapper, #edit-time-wrapper, #edit-medium-wrapper, #edit-subject-wrapper, #edit-details-wrapper").show();
    }
    if( $(this).val() != 0 ){
      $("#make-changes-wrapper").show();
    }else{
      $("#make-changes-wrapper").hide();
    }
  }).change();

  countBoxes();

  $("#ws-admissions-prospective-controls input.form-checkbox").change(function() {
    if( $(this).is(':checked') )
      $(this).parent().parent().addClass("checked");
    else $(this).parent().parent().removeClass("checked");
  }).change();

  $("#ws-admissions-prospective-form input.form-checkbox").change(function() {
    if( $(this).is(':checked') )
      $(this).parent().parent().parent().parent().addClass("checked");
    else $(this).parent().parent().parent().parent().removeClass("checked");
    countBoxes();
    } );

  var headerCount = $("#prospectives th").length;

  $("#prospectives").tablesorter( 
    {
      headers:
      { 0: { sorter: false},
        2: { sorter: 'lastName'},
        10: { sorter: 'countme'},
        11: { sorter: 'countme'}
      },
      widgets: ["zebra"],
    }
  );

  $("#prospectives").bind("sortStart",function() {
      $("#progressbar").css("display", "inline-block");
  }).bind("sortEnd",function() {
      $("#progressbar").hide();
      $("#ws-admissions-prospective-form table.sticky-header thead th").removeClass("headerSortDown headerSortUp");
      for( var i=1; (i<=headerCount); i++ ){
        if( $("#prospectives thead th:nth-child("+i+")").hasClass("headerSortUp") ){
          $("#ws-admissions-prospective-form table.sticky-header thead th:nth-child("+i+")").addClass("headerSortUp");
        }
        if( $("#prospectives thead th:nth-child("+i+")").hasClass("headerSortDown") ){
          $("#ws-admissions-prospective-form table.sticky-header thead th:nth-child("+i+")").addClass("headerSortDown");
        }
      }
  });

  var headerClick = $("#ws-admissions-prospective-form table.sticky-header thead th").click(function(){
    $("#prospectives thead th:nth-child("+(headerClick.index(this)+1)+")").click();
  });


  $("#prospectives th.checkbox").click(function() {
    var checks = $("#ws-admissions-prospective-form input.form-checkbox:checked").length;
    var boxes = $("#ws-admissions-prospective-form input.form-checkbox").length;
    if( checks==boxes ){
      $("#ws-admissions-prospective-form input.form-checkbox").attr("checked", false );
      $( "#prospectives tbody tr" ).removeClass("checked");
    }else{
      $("#ws-admissions-prospective-form input.form-checkbox").attr("checked", true );
      $( "#prospectives tbody tr" ).addClass("checked");
    }
    countBoxes();
    $("#progressbar").hide();
  });

  $("#ws-admissions-prospective-form").submit(function(){
    var checks = $("#prospectives input.form-checkbox:checked").length;
    if( checks == 0 ){
      return false;
    }
    var pl = "s";
    if( checks == 1 ) pl = "";
    var act = $("#edit-actions :selected").text();
    var medium = "";
    if( $("#edit-medium-wrapper").is(":visible") ){
      medium = " via " + $("#edit-medium :selected").text();
    }
    if( $("#edit-actions").val() != "email"){
      return confirm( checks + " case" + pl + ' will be updated with the action: ' + act + medium + '. \nContinue?' );
    }else{
      var fullname = $("#prospectives tr:has(input.form-checkbox:checked) td+td+td").children("div:first").html();
      nam = fullname.split('<i></i>');
      lastname = nam[2].replace( '</b>', '' );
      firstname = nam[0].replace( '<b>', '' );
      if( nam[1] == " " ){
        nickname = firstname;
      }else{
        nickname = nam[1].replace( '" ', '' );
        nickname = nickname.replace( ' "', '' );
      }
      var subject = $("input#edit-subject").val().replace(/{firstname}/gi, firstname);
      subject = subject.replace(/{lastname}/gi, lastname);
      subject = subject.replace(/{nickname}/gi, nickname);

      var body = $("#edit-details").val().replace(/{firstname}/gi, firstname);
      body = body.replace(/{lastname}/gi, lastname);
      body = body.replace(/{nickname}/gi, nickname);
      body = body.replace(/(\n|\r)/g, "<br />");

      $("span#email-dialog-count").html(checks+" student"+pl);
      $("span#email-dialog-to").html(firstname+" "+lastname);
      $("span#email-dialog-subject").html(subject);
      $("div#email-dialog-body").html(body);
      $("#edit-subject, #edit-details").removeClass("error");

      if (subject.search(/({|})/g) != -1){
        $("#edit-subject").addClass("error");
        alert("The message subject contains an invalid token. Correct tokens are {firstname}, {lastname}, and {nickname}");
      } else if(body.search(/({|})/g) != -1){
        $("#edit-details").addClass("error");
        alert("The message contains an invalid token. Correct tokens are {firstname}, {lastname}, and {nickname}");
      } else{

        $("#email-confirm-box").dialog({
          title: "Send "+checks+" Message"+pl+"?",
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
              $(this).dialog("destroy");
          },

          open:function() {
              $("#email-confirm-box").show();
          },

          buttons: {
              "Send": function() {
                  $(this).dialog("close");
                   $("#ws-admissions-prospective-form")[0].submit();

                },
              "Cancel": function() {
                  $(this).dialog("close");
                  $(this).dialog("destroy");
              }
          }
        });
      }
    }

  return false;
  });

});
