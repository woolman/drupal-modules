$(document).ready(function () {

  var spouseDefault = $(".spouse-name-here").html();

  $("input.same-address:checked").each(function() {
    $fs = $(this).parents('fieldset');
    $(".hide-address", $fs[0]).hide();
  });

  if ($("#edit-not-spouse-checkbox").attr("checked")==false) {
    $("#edit-not-spouse-select-wrapper").css('display','none');
  }

  if($("#spouse-name").length) {
    $(".spouse-name-here").html($("#spouse-name").html());
    if ($("#edit-not-spouse-checkbox").attr("checked") == false) {
      $("#edit-spouse-na-wrapper").hide().checked=false;
    }
  }

  if ($("#edit-spouse-na").is(":checked")) {
    $("#spouse-wrapper .all-fields, div[id$='relationship-spouse-wrapper']").hide();
    $("#edit-e-contact-na").attr('disabled', 'disabled');
  }

  if ($("#edit-e-contact-na").is(":checked")) {
    $("#e-contact-wrapper .all-fields").hide();
    $("#edit-spouse-na").attr('disabled', 'disabled');
  }

  var kids = $("#edit-number-of-children").val();
  if(kids < 10) {
    $("#children-wrapper fieldset:gt("+(kids-1)+")").hide();
  }

  $("#edit-spouse-last, #edit-spouse-first, #edit-spouse-nick").blur(function() {
    spouseName(spouseDefault);
  });

  $(".na-instructions-wrapper input:checkbox").change(function() {
    $fs = $(this).parents('fieldset');
    if ($(this).is(":checked")) {
      $(".all-fields", $fs[0]).hide(600);
      $(".na-instructions-wrapper input:checkbox").not(this).attr('disabled', 'disabled');
    }
    else {
      $(".all-fields", $fs[0]).show(600);
      $(".na-instructions-wrapper input:checkbox").not(this).removeAttr('disabled');
    }
    spouseName(spouseDefault);
  });

  $("input.same-address").change(function() {
    $fs = $(this).parents('fieldset');
    if ($(this).is(':checked')) {
      $(".hide-address", $fs[0]).hide(600);
    }
    else {
      $(".hide-address", $fs[0]).show(600);
    }
  });

  $("#edit-number-of-children").change(function() {
    if($(this).val() > kids) {
      $("#children-wrapper fieldset:lt("+($(this).val())+")").show(600);
    }
    else if($(this).val() < kids) {
      $("#children-wrapper fieldset:gt("+($(this).val()-1)+")").hide(600);
    }
    kids = $(this).val();
  });

  $("#edit-not-spouse-checkbox").change(function() {
    if ($(this).attr("checked")==true) {
      if(confirm("You are about to change your spouse to a different person. Continue?")) {
        $("#edit-not-spouse-select-wrapper").css('display','inline-block');
        $("#edit-not-spouse-checkbox-wrapper").addClass("selected");
        $("#spouse-instructions").html('Please enter information about your new partner or spouse, if applicable.');
        $('select[id$="relationship-spouse"]').val('');
        clear_form_elements("#spouse-wrapper fieldset");
        $("#edit-spouse-na-wrapper").show();
        $(this).css('opacity','0.6');
        $('#spouse-wrapper .change-msg').remove();
        spouseName(spouseDefault);
      }
      else {
        this.checked = false;
      }
    }
    else {
      this.checked = true;
    }
  });
  $("#edit-not-e-contact-checkbox").change(function() {
    if ($(this).attr("checked") == true) {
      if(confirm("This will change your family's emergency contact from "+$('#e-contact-name').html()+" to a different person. Continue?")) {
        $("#edit-not-e-contact-checkbox-wrapper").addClass("selected");
        $("#e-contact-instructions").html('Please enter information about your new emergency contact person.');
        $(this).css('opacity','0.6');
        $('#e-contact-wrapper .change-msg').remove();
        clear_form_elements("#e-contact-wrapper fieldset");
      }
      else {
        this.checked = false;
      }
    }
    else {
      this.checked = true;
    }
  });

  $('input[name$="first"], input[name$="last"]').keyup(function() {
    $(this).parents('fieldset').find('.change-msg').show(200);
    $(this).unbind('keyup');
  });

  $('select[name$="primary"]').change(function() {
    $(this).parents('.subset').find('span.primary, span.form-required').remove();
    var primary = $(this).val();
    var label = $(this).parents('.subset').find('label[for*="'+primary+'"]').html();
    $(this).parents('.subset').find('label[for*="'+primary+'"]').html(label+' <span class="form-required" title="This field is required.">*</span> <span class="primary">PRIMARY</span>');
  }).change();

  preventEnterSubmit('#woolman-camp-family-form');


  function spouseName(spouseDefault) {
    if($("#edit-spouse-last").val() != '' && $("#edit-spouse-na").attr("checked")!=true) {
      $('div[id$="relationship-spouse-wrapper"]').show();
      if($("#edit-spouse-nick").val() != '') {
        var firstName = $("#edit-spouse-nick").val();
      }else {
        var firstName = $("#edit-spouse-first").val();
      }
      $(".spouse-name-here").html(firstName+" "+$("#edit-spouse-last").val() );
    }else {
      $(".spouse-name-here").html(spouseDefault );
      $('div[id$="relationship-spouse-wrapper"]').hide();
    }
  }

  function clear_form_elements(ele) {
    $(ele).find(':input').each(function() {
      switch(this.type) {
        case 'checkbox':
        case 'radio':
          this.checked = false;
          break;
        case 'select-one':
          $(this).val('1228');
          break;
        default:
          $(this).val('');
      }
    });
    $(ele + " .hide-address").show();
  }

  function restore_form_elements(ele) {
    $(ele).find(':input').each(function() {
      switch(this.type) {
        case 'checkbox':
        case 'radio':
          this.checked = $(this).prop('defaultChecked');
          break;
        default:
          $(this).val($(this).attr('value'));
      }
    });
    $(ele + " .same-address").change();
  }

});
