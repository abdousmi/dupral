<?php

/* @file
 * API and hooks documentation for the Commerce Payment module.
 */

/**
 * Some variables are not setted by the contrib module
 * but can be set in your custom module to change some
 * commerce payline module behaviours
 *
 * We give some example in the following hook_install pattern
 * Note you can also implement these variable settings process
 * in hook_update_N or in submit form process for instance
 */

function hook_install() {
  // disable the payline js behaviour on payment checkout pane
  // can be used to debug process for axample
  variable_set('commerce_paylinebymonext_nojs', 1);
  // set the search interval for the direct link to payline transacation dashboard to 1800 second
  // total interval will be one hour
  // @see commerce_paylinebymonext_ui_remote_id_field_format();
  variable_set('commerce_paylinebymonext_transaction_search_interval', 1800);
  // disable the refund process in ui
  variable_set('commerce_paylinebymonext_ui_refund_disabled', 1);
  // set the return url after a refund process
  variable_set('commerce_paylinebymonext_ui_refund_result_redirect', 'admin/commerce/orders/' . $order->order_id . '/payment/');
}

/**
 * Alter return urls before the WS payment is called to payline
 *
 * @param array $urls
 */
function hook_commerce_paylinebymonext_interface_url_alter(&$urls) {
  $urls = array(
    'cancelURL' => 'my_custom_cancel_url',
    'notificationURL' => 'my_custom_notification_url',
    'returnURL' => 'my_custom_notification_url',
  );
}

/**
 * Alter payment method settings
 *
 * @param array $settings
 */
function hook_commerce_paylinebymonext_settings_alter(&$settings) {
  if (!isset($settings['primary_contracts']) || empty($settings['primary_contracts'])) {
    $contracts = commerce_paylinebymonext_contracts_get($settings['method_id']);
    $settings['primary_contracts'] = $contracts['primary_contracts'];
    $settings['secondary_contracts'] = $contracts['secondary_contracts'];
  }
}

/**
 * Give opportunity to change informations about contract type
 * @see commerce_paylinebymonext_contracts_types().
 *
 * @param array $contracts
 */
function hook_commerce_paylinebymonext_contracts_types_alter(&$contracts) {
  // alter the dinner contract information
  $contracts['dinner'] = array(
    'name' => 'Diners MyWay Name',
    'path' => drupal_get_path('module', 'my_module') . '/images/DINERS.gif',
  );
}

/**
 * @param $params
 */
function hook_commerce_paylinebymonext_params_alter(&$params) {

}

/**
 * Alter the data will be stored in the transaction object during a payline transaction (payment or refund) save process
 *
 * @param array $data
 *  the data array for a transaction
 * @param array $response
 *  the ws response array
 * @param array $payment_method
 *  the full loaded payment method instance
 * @param stdClass $order
 *  a commerce order object
 */
function hook_commerce_paylinebymonext_transaction_data_alter(&$data, $response, $payment_method, $order) {
  // no example
}

/**
 * Alter the message and its variables will be stored in the transaction object during a payline transaction (payment or refund) save process
 *
 * @param array $message
 *  array with one value by message line
 * @param array $message_variables
 *  transaction message_variables array
 * @param array $response
 *  the ws response array
 * @param array $payment_method
 *  the full loaded payment method instance
 * @param stdClass $order
 *  a commerce order object
 */
function hook_commerce_paylinebymonext_transaction_message_alter(&$message, &$message_variables, $response, $payment_method, $order) {
  // no example
}

/**
 * Act on checkout completion
 *
 * @param object $order
 */
function hook_commerce_paylinemonext_aoc($order) {
  /**
  $wrapper = entity_metadata_wrapper('commerce_order', $order);

  $wrapper->status->set('authorized');
  $wrapper->save();

  return $order;
  */
}