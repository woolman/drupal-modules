(function($, D) {

  // parser to order name field by last name (requires <i></i> in-between the first, middle & last names)
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

  /**
   * In-place editing for event status
   */
  function statusMenu(ele) {
    ele.unbind('click').removeClass('r').wrapInner('<span style="display:none" />');
    var status_id = ele.attr('data-sid');
    var options = D.settings.camp_staff.status;
    var items = '';
    for (var o in options) {
      items += '<option ' + (status_id == o ? 'selected="selected"' : '') + ' value="' + o + '">' + options[o] + '</option>';
    }
    ele.prepend('<select>' + items + '</select>');
    $('select', ele).change(function() {
      var status_id = $(this).val();
      var status = $('option:selected', this).text();
      var oldStatus = '';
      $('option', this).each(function() {
        if($(this).attr('defaultSelected')) {
          oldStatus = $(this).text();
        }
      });
      var ele = $(this).parent();
      var pid = ele.attr('data-pid');
      var row = $(this).parents('tr');
      var name = row.find('td:first').text();
      var confText = "Change " + name + "'s status to " + status + "?";
      if (confirm(confText)) {
        $.get('/staff/camp/js', {op: "change_status", sid: status_id, pid: pid}, function(data) {
          if (data == '' || data == 'error' || !data) {
            alert('Sorry, an error occurred. Refresh this page and try again.');
            $('option', this).each(function() {
              $(this).attr('selected', $(this).attr('defaultSelected'));
            });
          }
          else {
            var text = $('span', ele).text().replace(oldStatus, status);
            $('select, span', ele).remove();
            ele.prepend(text);
            ele.attr('class', data + ' p-s r');
            ele.attr('data-sid', status_id);
            if (data != 'negative') {
              row.removeClass('negative');
            }
            else {
              if ($('span.p-s', row).length < 2) {
                row.addClass('negative');
              }
            }
            ele.click(function() {
              statusMenu($(this));
            });
            row.parents('table').trigger('update');
          }
        }, 'json');
      }
      else {
        // Reset on user cancel
        $('select', ele).remove();
        $('span', ele).replaceWith($('span', ele).text());
        ele.addClass('r').click(function() {
          statusMenu($(this));
        });
      }
    });
  }

  D.behaviors.woolman_camp_staff = function(context) {

    var headerCount = $(".tablesorter:first th").length;

    $(".tablesorter").not('.sorted').tablesorter({
      headers: {
        0: {sorter: 'lastName'},
        5: {sorter: 'lastName'},
        10: {sorter: false}
      },
      widgets: ["zebra"],
    }).bind("sortEnd", function() {
      $(this).prev().find("thead th").removeClass("headerSortDown headerSortUp");
      for(var i=1; (i<=headerCount); i++) {
        if($(this).find("thead th:nth-child("+i+")").hasClass("headerSortUp")) {
          $(this).prev().find("thead th:nth-child("+i+")").addClass("headerSortUp");
      }
              if($(this).find("thead th:nth-child("+i+")").hasClass("headerSortDown")) {
          $(this).prev().find("thead th:nth-child("+i+")").addClass("headerSortDown");
        }
      }
    }).addClass('sorted');

    // When clicking on the sticky header, trigger a click on the tablesorter
    $("table.sticky-header:not('.n-sync') th").click(function() {
      var sticky = $(this).parents("table");
      var child = $("thead th", sticky).index(this) + 1;
      sticky.next(".tablesorter").find("th:nth-child("+ child +")").click();
    });
    $("table.sticky-header:not('.n-sync')").addClass('n-sync');

    $(".shuttles-outer .shuttle").not('.open, .closed, .empty').each(function() {
      var shuttle = $(this);
      shuttle.addClass('closed');
      $('ul', shuttle).hide();
      $('h5', shuttle).click(function() {
        var shuttle = $(this).parent();
        if (shuttle.hasClass('closed')) {
          shuttle.removeClass('closed').addClass('open').find('ul').show(200);
        }
        else {
          shuttle.removeClass('open').addClass('closed').find('ul').hide(200);
        }
      });
    });

    $('span.p-s').not('.r').addClass('r').click(function() {
      statusMenu($(this));
    });

    // Manage discounts form
    $('#woolman-camp-manage-discount [name$="[type]"]:not(".ready")', context).addClass('ready').change(function() {
      var row = $(this).parents('tr');
      if ($(this).val() == '') {
        $('.new > .form-item, .today', row).css('visibility', 'hidden');
        $('input[type=text]', row).attr('disabled', 'disabled');
        $('.date-info', row).css('text-decoration', 'line-through');
      }
      else {
        $('.new > .form-item, .today', row).css('visibility', 'visible');
        $('input[type=text]', row).removeAttr('disabled');
        $('.date-info', row).removeAttr('style');
      }
      discountCalc();
    }).change();
    $('#woolman-camp-manage-discount input[type=text]:not(".ready")', context).addClass('ready').change(discountCalc).keyup(discountCalc);

    function discountCalc() {
      var base = parseFloat($('#discount-form-base').html());
      var paid = parseFloat($('#discount-form-paid').html());
      var subTotal = base;
      $('input[type=text]:not(:disabled)', '#woolman-camp-manage-discount').each(function() {
        subTotal -= $(this).val();
      });
      if (subTotal == 0) {
        subTotal = '0';
      }
      var balance = subTotal - paid;
      if (balance == 0) {
        balance = '0';
      }
      $('#discount-form-subtotal').html(subTotal);
      $('#discount-form-balance').html(balance);
    }

  };
})(jQuery, Drupal);
