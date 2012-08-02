var wsStaff = function($, D, undefined) {

  var pub = {};
  var timeStamp;

  /**
  * Display view box for an activity
  */
  pub.viewActivity = function(ele, row) {
    $("#activity-content").html('<div class="loading"> </div>');

    $("#view-activity").dialog({
      title: actTitle(ele, row),
      modal: true,
      width : "680px",
      height: "590px",
      resizable: false,
      bgiframe: false,
      overlay: {
        opacity: 0.5,
        background: "black"
      },

      beforeclose: function(event, ui) {
        $(this).dialog("destroy");
        $('#page').removeClass('busy');
      },

      open:function() {
        $("#view-activity").show();
        $('.ui-dialog-buttonpane button + button').hide();
        var cid = row.attr('data-contact-id');
        var aid = ele.attr('data-aid');
        $("#activity-content").load("/civicrm/case/activity/view?snippet=4&cid=" + cid + "&aid=" + aid, function(response, status) {
          if (status == "error") {
            $("#view-activity").dialog("close");
          }
          else {
            $('.ui-dialog-buttonpane button').show();
          }
        });
      },

      buttons: {
        "Close": function() {
          $(this).dialog("close");
        },
        "Edit Activity": function() {
          $(this).dialog("close");
          pub.createActivity(ele, row);
        }
      }

    });
  }

  /**
  * Display edit box for an activity
  */
  pub.createActivity = function(ele, row) {
    $("#activity-content").html('<div class="loading"> </div>');
    var atype = ele.attr('data-atype');
    var aid = ele.attr('data-aid') || 0;
    var case_id = row.attr('data-case-id');
    var contact_id = row.attr('data-contact-id');
    var editor = null;
    var save = aid == 0 && atype == 3 ? 'Send' : 'Save';
    var buttons = {};
    buttons['Cancel'] = function() {
      $(this).dialog("close");
    };
    buttons[save] = function() {
      var ok = true;
      if (editor !== null) {
        $('[name="params_details"]').val(editor.getData());
      }
      // Check required fields
      $('.form-required', "#activity-content form").parent().parent().find(':input').each(function() {
        if ($(this).val() == '') {
          $(this).addClass('error').focus(function() {
            $(this).removeClass('error');
          });
          ok = false;
        }
      });
      if (ok === false) {
        alert('Please complete required fields.');
        return;
      }
      $('.ui-dialog-buttonpane button + button, #activity-content form').hide();
      $("#activity-content").prepend('<div class="loading"></div>');
      var ajax = true;
      $('#view-activity [type=file]').each(function() {
        if ($(this).val()) {
          ajax = false;
        }
      });
      if (ajax) {
        $.ajax({
          type: 'POST',
          dataType: 'json',
          url: '/staff/admissions/js?op=activity_form_submit&sem=' + encodeURIComponent(D.settings.ws_staff.semester) + '&ts=' + encodeURIComponent(timeStamp),
          data: $("#activity-content form").serialize(),
          success: function(data) {
            if (editor !== null) {
              editor.destroy();
              editor = null;
            }
            $("#view-activity").dialog('close');
            applyUpdates(data);
          },
          error: function() {
            $('.ui-dialog-buttonpane button, #activity-content form').show();
            $('#view-activity .loading').remove();
          }
        });
      }
      else {
        $('#view-activity form')[0].submit();
      }
    };
    $("#view-activity").dialog({
      title: actTitle(ele, row),
      modal: true,
      width : "680px",
      height: "590px",
      resizable: false,
      bgiframe: false,
      overlay: {
        opacity: 0.5,
        background: "black"
      },

      beforeclose: function(event, ui) {
        $(this).dialog('destroy');
        $('#page').removeClass('busy');
      },

      open: function() {
        $("#view-activity").show();
        $('.ui-dialog-buttonpane button + button').hide();
        $("#activity-content").load('/staff/admissions/js?op=activity_form&case_id=' + case_id + '&contact_id=' + contact_id + '&atype=' + atype + '&aid=' + aid, function(response, status) {
          if (status == "error") {
            $("#view-activity").dialog("close");
            return;
          }
          $('.datepicker', "#activity-content").datepicker();
          attachBehaviors($("#view-activity"));
          if ($('[name="params_details"]').length > 0 && (atype !== '3' || aid)) {
            editor = CKEDITOR.replace('params_details', {
              toolbar : [
                [ 'Bold', 'Italic', 'Strike', '-',
                  'Outdent','Indent', '-',
                  'NumberedList', 'BulletedList', '-',
                  'Link', 'Unlink', '-',
                  'Format', '-',
                  'RemoveFormat', '-',
                  'PasteFromWord' ]
                ]
              }
            );
          }
          $('.ui-dialog-buttonpane button').show();
        });
      },

      buttons: buttons
    });
  }

  /**
   * Actions menu
   */
  pub.actionMenu = function(ele, row) {
    $('#action-menu').remove();
    var acts = {
      3: 'Send an Email',
      14: 'Follow-Up By Staff',
      28: 'Communication from Student',
      29: 'Send more Info',
    };
    var menu = '<ul id="action-menu" style="display:none">';
    for (var act in acts) {
      menu += '<li><a href="#" data-atype="' + act + '" data-fn="createActivity">' + acts[act] + '</a></li>';
    }
    ele.after(menu + '</ul>');
    attachBehaviors('#action-menu');
    $('#action-menu').fadeIn(190);
  }

  /**
   * In-place editing select menu
   */
  function selectMenu(ele) {
    ele.unbind('click').removeClass('ready').wrapInner('<span style="display:none" />');
    var id = ele.attr('data-id');
    var type = ele.attr('data-type');
    var options = D.settings.ws_staff[type];
    var items = '';
    for (var o in options) {
      items += '<option ' + (id == o ? 'selected="selected"' : '') + ' value="' + o + '">' + options[o] + '</option>';
    }
    ele.prepend('<select>' + items + '</select>');
    $('select', ele).change(function() {
      var ele = $(this).parent();
      var id = $(this).val();
      var label = $('option:selected', this).text();
      var type = ele.attr('data-type');
      var row = $(this).parents('tr');
      var case_id = row.attr('data-case-id');
      var contact_id = row.attr('data-contact-id');
      if (confirm("Change " + rowName(row) + "'s " + type + " to " + label + "?")) {
        $.get('/staff/admissions/js', {op: type, case_id: case_id, contact_id: contact_id, selected: id, ts: timeStamp, sem: D.settings.ws_staff.semester}, function(data) {
          if (data == '' || data == 'error' || !data) {
            setMsg('Sorry, an error occurred. Refresh this page and try again.');
            $('option', this).each(function() {
              $(this).attr('selected', $(this).attr('defaultSelected'));
            });
          }
          else {
            applyUpdates(data);
          }
        }, 'json');
      }
      else {
        // Reset on user cancel
        $('select', ele).remove();
        $('span', ele).replaceWith($('span', ele).text());
        ele.addClass('ready').click(function() {
          selectMenu($(this));
        });
      }
    });
  }

  /**
   * Variables for relative dates
   */
  var DAY = 24 * 60 * 60 * 1000,
    WEEK = 7 * DAY,
    YEAR = DAY * 365,
    MONTH = YEAR / 12;

  var formats = {
    short: [
      [ 0 - DAY, ' days', 0 - DAY, 'In ' ],
      [ 0, 'In 1 day' ],
      [ DAY, 'Today' ],
      [ 7 * DAY, ' dy ago', DAY ],
      [ 3.5 * WEEK, ' wk ago', WEEK ],
      [ 11.5 * MONTH, ' mo ago', MONTH ],
      [ Number.MAX_VALUE, ' yr ago', YEAR ]
    ],
    long: [
      [ 0 - DAY, ' days', 0 - DAY, 'In ' ],
      [ 0, 'Tomorrow' ],
      [ DAY, 'Today' ],
      [ 2 * DAY, 'Yesterday' ],
      [ 7 * DAY, ' days ago', DAY ],
      [ 1.5 * WEEK, '1 week ago'],
      [ 3.5 * WEEK, ' wks ago', WEEK ],
      [ 1.5 * MONTH, '1 month ago' ],
      [ 11.5 * MONTH, ' mos ago', MONTH ],
      [ 1.5 * YEAR, '1 year ago' ],
      [ Number.MAX_VALUE, ' yrs ago', YEAR ]
    ]
  };

  var today = new Date();
  var now = new Date(today.getFullYear(), today.getMonth(), today.getDate()).getTime();

  /**
   * Recalculate reltive dates when day has changed
   */
  function checkDate() {
    var today = new Date();
    var check = new Date(today.getFullYear(), today.getMonth(), today.getDate()).getTime();
    if (check != now) {
      now = check;
      attachBehaviors('#page');
    }
  }

  /**
   * Calculate relative date from a m/d/yy string
   */
  pub.relDate = function(str, dsply) {
    var parts = str.split('/');
    var then = new Date('20' + parts[2], parts[0] - 1, parts[1]).getTime();
    var delta = now - then,
      f, i, len;

    for (i = -1, len=formats[dsply].length; ++i < len;) {
      f = formats[dsply][i];
      if (delta < f[0]){
        return (f[3] || '') + (f[2] == undefined ? f[1] : Math.round(delta/f[2]) + f[1]);
      }
    };
  }

  var loading = false;

  /**
   * Get latest info from the server
   */
  function fetchUpdates() {
    // Only load if another request isn't already running
    if (loading === false) {
      $.get('/staff/admissions/js', {op: 'update_check', ts: timeStamp, sem: D.settings.ws_staff.semester},
        function(data) {
          applyUpdates(data);
        }, 'json'
      );
    }
  }

  /**
   * Update tables with new info from the server
   */
  function applyUpdates(data) {
    if (data['ts']) {
      timeStamp = data['ts'];
      if (data['rows']) {
        for (var i in data['rows']) {
          setMsg(data['rows'][i]['msg']);
          var semester = data['rows'][i]['semester'];
          var case_id = data['rows'][i]['case_id'];
          var table = $('table[data-semester="' + semester + '"]');
          var row = '';
          if (table.length > 0) {
            row = $('tr[data-case-id="' + case_id + '"]', table);
          }
          // If row belongs on this page
          if ($.inArray(data['rows'][i]['status'], D.settings.ws_staff.displayStatus) > -1) {
            // Row doesn't exist or is in the wrong place
            if (row.length < 1) {
              var oldRow = $('tr[data-case-id="' + case_id + '"]');
              if (oldRow.length > 0) {
                var table = oldRow.parents('table');
                oldRow.remove();
                tableNumbers(table);
              }
              $('table[data-semester="' + semester + '"] tbody').append(data['rows'][i]['row']);
              tableNumbers($('table[data-semester="' + semester + '"]'), true);
            }
            // Row is in the right table, just replace it
            else {
              row.replaceWith(data['rows'][i]['row']);
              tableNumbers(table, true);
            }
            attachBehaviors($('tr[data-case-id="' + case_id + '"]'));
          }
          // If row does not belong on this page
          else {
            var row = $('tr[data-case-id="' + case_id + '"]');
            if (row.length > 0) {
              row.replaceWith(data['rows'][i]['row']);
              var newRow = $('tr[data-case-id="' + case_id + '"]');
              newRow.fadeOut(900, function(){
                var table = newRow.parents('table');
                newRow.remove();
                tableNumbers(table);
              });
            }
          }
        }
      }
    }
  }

  /**
   * Display a message for 10 seconds
   */
  var msgCount = 0
  function setMsg(msg, cls) {
    cls = cls || 'msg';
    var id = 'msg-' + ++msgCount;
    $('#staff-msg').append('<div id="' + id + '" class="' + cls + '"><p>' + msg + '</p></div>');
    window.setTimeout("$('#" + id + "').animate({opacity: 0, height: 0}, 600, function(){$('#" + id + "').remove()})", 10000);
  }

  /**
   * Title bar for an activity dialog
   */
  function actTitle(ele, row) {
    var type = ele.attr('data-atype');
    var title = D.settings.ws_staff.actTypes[type];
    return title + ' for ' + rowName(row);
  }

  /**
   * Contact name from column 2
   */
  function rowName(row) {
    var name = row.find('td:first + td').text();
    var index = name.indexOf('(');
    if (index >= 0) {
      name = name.substr(index+1).replace(')', '');
    }
    return name;
  }

  /**
   * Put numbers in the first column after sorting a table
   * Update tablesorter if new data has been added
   */
  function tableNumbers(table, trigger) {
    if ($(table).length > 0) {
      $('tbody tr', table).each(function(i) {
        $('td:first', this).html(i+1);
      });
    }
    if (trigger === true) {
      table.trigger('update');
    }
  }

  /**
   * Parser to order name field by last name (requires <i></i> in-between the first, middle & last names)
   */
  $.tablesorter.addParser({
    id: 'lastname',
    is: function(s) {
      return false;
    },
    format: function(s) {
      var val = s.toLowerCase().split('<i></i>');
      return val[2]+val[0];
    },
    type: 'text'
  });

  /**
   * Sync sticky header to match the real header
   */
  function headerSync(table) {
    table.prev().find("thead th").each(function(i) {
      $(this).attr('class', $("thead th:nth-child(" + (i+1) + ")", table).attr('class'));
    });
  }

  /**
  * Initialize some stuff
  */
  $(document).ready(function() {
    timeStamp = D.settings.ws_staff.timeStamp;

    // Fetch updates from server every 5 min
    window.setInterval(fetchUpdates, 300000);

    // Check date every 15 min
    window.setInterval(checkDate, 900000);

    $('#page').ajaxStart(function() {
      loading = true;
      $('#staff-msg .err').remove();
      $(this).addClass('busy');
    }).ajaxStop(function() {
      loading = false;
      $(this).removeClass('busy');
    }).ajaxError(function() {
      setMsg('Unable to reach Woolman server.<br />Check your internet connection.', 'err');
    });

    $("table.tablesorter").tablesorter({
      headers: {
        0: { sorter: false},
        1: { sorter: 'lastName'}
      },
      widgets: ["zebra"],
    }).bind("sortEnd", function() {
      tableNumbers($(this));
      headerSync($(this));
      $('#page').removeClass('busy');
    });

    $("table.tablesorter").each(function() {
      headerSync($(this));
    });

    // When clicking on the sticky header, trigger a click on the tablesorter
    $("table.sticky-header thead th").click(function() {
      var sticky = $(this).parents("table");
      var child = $("thead th", sticky).index(this) + 1;
      sticky.next(".tablesorter").find("th:nth-child("+ child +")").click();
    });

    $('body').click(function() {
      $('#action-menu').hide();
    });
  });

  /**
  * Attach behaviors to ajax elements
  */
  D.behaviors.ws_staff = function(context) {
    $('div.sel', context).not('.ready').addClass('ready').click(function() {
      selectMenu($(this));
    });

    $('a[href="#"]', context).not('.ready').addClass('ready').click(function() {
      var fn = $(this).attr('data-fn');
      $('#action-menu').hide();
      pub[fn]($(this), $(this).parents('tr'));
      return false;
    });

    $('.abs-dt', context).each(function() {
      var dsply = $(this).parents('td').hasClass('cell') ? 'short' : 'long';
      var rel = pub.relDate($(this).text(), dsply);
      $(this).next('.rel-dt').html(rel);
    });
  };

  /**
  * Todo - there seems to be a bug in D6 that broke tablesorter when attaching its behavior,
  * So here's a workaround (modified version of Drupal.attachBehaviors())
  */
  function attachBehaviors(context) {
    context = context || document;
    var behaviors = D.behaviors;
    // Execute all of them except tableheader.
    behaviors.tableHeader = function(){};
    $.each(behaviors, function() {
      this(context);
    });
  }

  return pub;
}(jQuery, Drupal);

//supports civi's revision functions
var cj = cj || jQuery;
