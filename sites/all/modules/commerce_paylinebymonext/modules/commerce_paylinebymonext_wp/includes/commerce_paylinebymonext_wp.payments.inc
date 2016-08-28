<?php

/**
 * @file
 *  Implements callback functions for the payment method.
 */

/**
 * Payment method callback: settings form.
 *
 * @param array $settings
 *
 * @return array
 *  form elements for the payment method's settings form included
 * as part of the payment method's enabling action in Rules
 */
function commerce_paylinebymonext_wp_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_paylinebymonext_ui', 'includes/commerce_paylinebymonext_ui.form');
  $settings['method_id'] = 'payline_wp';
  drupal_alter('commerce_paylinebymonext_settings', $settings);
  $form = commerce_paylinebymonext_ui_settings_form($settings);
  $form['mode'] = array(
    '#type' => 'value',
    '#value' => 'CPT',
  );
  $form['action'] = array(
    '#type' => 'value',
    '#value' => 101,
  );
  return $form;
}

/**
 * Payment method callback: submit form
 *  render sub form on checkout pane
 *
 * @param array $payment_method
 * @param array $pane_values
 * @param array $checkout_pane
 * @param object $order
 * @return array
 */
function commerce_paylinebymonext_wp_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.payments');
  $form = commerce_paylinebymonext_checkout_offsite_form($payment_method);
  return $form;
}

/**
 * Implements hook_submit_form_submit().
 *   store selected contract number to expose for the following of process
 *
 * @param array $payment_method
 * @param array $pane_form
 * @param array $pane_values
 * @param object $order
 * @param $charge
 */
function commerce_paylinebymonext_wp_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  commerce_paylinebymonext_order_data_set($order, array('commerce_paylinebymonext_contract_number' => $pane_values['commerce_paylinebymonext_contract']));
}

/**
 * Implements hook_redirect_form().
 *  Do the following  process in the correct hook to stay in commerce process :
 *  - call payline ws
 *  - if ok get a redirect (and no post as commerce habitually do it)
 *  - if not :
 *  -- log and call commerce_payment_redirect_pane_previous page to set order on correct step
 *  -- do a drupal go to on previous step as commerce_payment_redirect_pane_previous doesn't do...
 *
 * @param array $form
 * @param array $form_state
 * @param object $order
 * @param array $payment_method
 * @return bool
 */
function commerce_paylinebymonext_wp_redirect_form($form, &$form_state, $order, $payment_method) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.payments');
  $response = commerce_paylinebymonext_doWebPayment($payment_method, $order, $order->data['commerce_paylinebymonext_contract_number']);
  if ($response['result']['code'] == '00000') {
    commerce_paylinebymonext_order_data_set($order, array('commerce_paylinebymonext_token' => $response['token']));
    drupal_goto($response['redirectURL']);
  }
  else {
    $args = array(
      '@code' => $response['result']['code'],
      '@short' => $response['result']['shortMessage'],
      '@long' => $response['result']['longMessage'],
    );
    watchdog(COMMERCE_PAYLINE_WD, 'the payline call failed with code : @code / shortMessage : @short / longMesage : @long', $args, WATCHDOG_ERROR);
    commerce_paylinebymonext_set_warning($order);
    commerce_paylinebymonext_redirect_pane_previous_page($order);
  }
  return FALSE;
}
