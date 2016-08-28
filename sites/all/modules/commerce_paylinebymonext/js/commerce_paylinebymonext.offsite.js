(function (jQuery) {

  /**
   * Handle the customer checkout payline form behaviour
   */

  Drupal.behaviors.commercePaylineByMonext = Drupal.behaviors.commercePaylineByMonextByMonext || {};
  Drupal.behaviors.commercePaylineByMonext.attach = function (context, settings) {
    this.hideRadio(context, settings);
  }

  /**
   * hide contracts type radio input and add handler on change to auto submit form on contract choice
   *
   * @param context
   * @param settings
   */
  Drupal.behaviors.commercePaylineByMonext.hideRadio = function (context, settings) {
    var $contracts = jQuery('.commerce-payline-contracts');
    $contracts.find('input').hide();
    $contracts.css('max-width', '210px');
    var $items = $contracts.find('.form-item').css('float', 'left').css('margin-right', '5px');
    var $labels = $items.find('label').css('cursor', 'pointer');
    $contracts.siblings('.description').css('clear', 'both');

    Drupal.behaviors.commercePaylineByMonext.autoSubmit(context, settings, $labels);
  }

  /**
   * Auto submit form on change event for elt
   *
   * @param context
   * @param settings
   * @param elt
   */
  Drupal.behaviors.commercePaylineByMonext.autoSubmit = function (context, settings, elt) {
    elt.once(function () {
      jQuery(this).click(function () {
        jQuery(this).siblings('input').attr('checked', 'checked').parents('form').submit();
      });
    });
  }

})(jQuery);