(function ($, D) {

  D.behaviors.woolman_camp_status = function(context) {
    preventEnterSubmit('#woolman-camp-med-form', context);
  };

}(jQuery, Drupal));
