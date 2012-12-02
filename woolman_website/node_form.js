(function ($, D) {

  $(document).ready(function() {
    /**
     * Callback when path alias is updated
     */
    function editPath() {
      var site = $('#woolman-path-site').val();
      var path = $('#woolman-path-path').val();
      var description = 'None';
      if (path.length || site.length) {
        // Limit to url-friendly characters
        path = path
          .toLowerCase()
          .replace(/[+&]+/g, '+')
          .replace(/[^a-z0-9.+\/]+/g, '-')
          .replace(/(\/{2,}|\/[.\-+]+|[.\-+]\/+)/g, '/');
        // Path must not start with a slash
        while (path.charAt(0) == '/' || path.charAt(0) == '+' || path.charAt(0) == '.' || path.charAt(0) == '-') {
          path = path.substr(1);
        }
        $('#woolman-path-path').val(path);
        if (path.length > 0) {
          // Allow a trailing slash while the user is typing but not in the final product
          var lastIndex = path.length - 1;
          if (path.charAt(lastIndex) == '/') {
            path = path.substr(0, lastIndex);
          }
          // If both site and path exist, insert slash between them
          if (site.length && path.length) {
            site += '/';
          }
        }
        if (path.length || site.length) {
          description = 'http://' + $('#woolman-path-site option:selected').text();
          if (path.length) {
            description += '/' + path;
          }
        }
      }
      // Display url
      $('#woolman-url').html(description);
      $('.vertical-tabs-list-path .summary').html(description.replace('http://', ''));
      // Set internal path
      $('#edit-path').val(site + path);
    }

    /**
     * User-friendly way of editing node path alias
     */
    if ($('#edit-path').length && $('#edit-pathauto-perform-alias').length < 1) {
      var sites = {
        '': 'woolman.org',
        '~semester': 'semester.woolman.org',
        '~camp': 'camp.woolman.org',
        '~blog': 'blog.woolman.org',
      }
      $('#edit-path-wrapper *').hide();
      $('#edit-path-wrapper').append(
        '<div class="subset" id="woolman-path"></div>' +
        '<div class="description" id="woolman-url">None</div>');
      var cont = $('#woolman-path');
      cont.append(
        '<div class="form-item"><label>Site</label><select id="woolman-path-site"></select></div>' +
        '<div class="form-item"><label>Path</label><input id="woolman-path-path" class="form-text" type="text" size="32"></input></div>');
      for (var i in sites) {
        $('select', cont).append('<option value="'+i+'">' + sites[i] + '</option>');
      }
      var existing = $('#edit-path').val();
      if (existing) {
        if (existing.charAt(0) == '~') {
          var slash = 1 + existing.indexOf('/');
          if (slash) {
            $('#woolman-path-site').val(existing.substring(0, slash - 1));
            $('#woolman-path-path').val(existing.substring(slash));
          }
          else {
            $('#woolman-path-site').val(existing);
          }
        }
        else {
          $('#woolman-path-path').val(existing);
        }
      }
      editPath();
      $('#woolman-path-path').keyup(editPath);
      $('#woolman-path-site').change(editPath);
    }
  });

  /**
   * Inline image rotation
   */
  D.behaviors.woolman_img_rot = function(context) {
    $(".insert select.insert-style", context).not(".rot").addClass("rot").change(function() {
      var img = $(this).parents(".filefield-element").find(".imagefield-preview img");
      img.removeClass("rotate-90-l").removeClass("rotate-90-r");
      if ($(this).val() == "imagecache_Rotate_Left") {
        img.addClass("rotate-90-l");
      }
      else if ($(this).val() == "imagecache_Rotate_Right") {
        img.addClass("rotate-90-r");
      }
    });
  };
}(jQuery, Drupal));
