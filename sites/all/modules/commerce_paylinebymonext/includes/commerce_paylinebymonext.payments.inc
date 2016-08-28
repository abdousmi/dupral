<?php

/**
 * @file
 *  Provide api function to handle payments features
 */

/**
 * Do a doWebPayment Call to Payline Server
 *
 * @param array $payment_method
 *  The full loaded payment method
 * @param stdClass $order
 *  A commerce_order object
 * @param string $contract_number
 * @return array
 *  The Payline Server Response
 */
function commerce_paylinebymonext_doWebPayment($payment_method, $order, $contract_number = NULL) {
  global $language;
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.contracts');
  // Make sure contract settings are available in payment methods settings
  // and give opportunity for custom module to add some particular settings
  $settings = $payment_method['settings'];
  $settings['method_id'] = $payment_method['method_id'];
  drupal_alter('commerce_paylinebymonext_settings', $settings);
  // Get a commercePaylineSDK object
  $payline = commerce_paylinebymonext_sdk($settings);
  // Get the payment, customer and order params from current order
  $payment_params = commerce_paylinebymonext_params_payment($settings, $order, $contract_number);
  $customer_params = commerce_paylinebymonext_params_customer($order);
  $order_params = commerce_paylinebymonext_params_order($order, $payment_params);
  // Get customisation params from payment method settings
  $customisation = $settings['customisation'];
  // Prepare contracts number params from payment method settings
  $contracts_number = commerce_paylinebymonext_contracts_number($settings);
  // format contract numbers for BO or FO behaviours
  // only one contract number has to be passed in the 'contracts" params
  // and contractNumber key in payment param has always to be filled
  if (!isset($payment_params['contractNumber']) || empty($payment_params['contractNumber'])) {
    $payment_params['contractNumber'] = array_shift($contracts_number['primary_contracts']);
    $contracts_number['secondary_contracts'] = array_merge($contracts_number['secondary_contracts'], $contracts_number['primary_contracts']);
  }
  $contracts_number['primary_contracts'] = array(
    $payment_params['contractNumber'],
  );
  // Set the full params array
  // and add the drupal/payline interface url
  $params = array(
    'payment' => $payment_params,
    'customPaymentTemplateURL' => !empty($customisation['custom_template_url']) ? $customisation['custom_template_url'] : NULL,
    'customPaymentPageCode' => !empty($customisation['custom_customisation_id']) ? $customisation['custom_customisation_id'] : NULL,
    'contracts' => !empty($contracts_number['primary_contracts']) ? $contracts_number['primary_contracts'] : NULL,
    'secondContracts' => !empty($contracts_number['secondary_contracts']) ? $contracts_number['secondary_contracts'] : NULL,
    'languageCode' => drupal_multilingual() ? strtoupper($language->language) : '',
    'securityMode' => 'SSL',
    'buyer' => $customer_params['buyer'],
    'billingAddress' => $customer_params['billingAddress'],
    'shippingAddress' => $customer_params['shippingAddress'],
    'order' => $order_params,
  ) + commerce_paylinebymonext_get_interface_url($settings['method_id'], $order->order_id);
  // give opportunity to alter $params before call payline
  drupal_alter('commerce_paylinebymonext_params', $params, $order);
  // give opportunity to alter paylineSDK
  drupal_alter('commerce_paylinebymonext_paylineSDK', $payline, $order);
  // WS call and log it
  $response = $payline->doWebPayment($params);
  $payline->saveLog();
  return $response;
}

/**
 * Return the payment params for WS call
 *
 * @param array $settings
 *  full payment methods settings
 * @param stdClass $order
 *  a commerce_order object
 * @param string $contract_number
 * @return array
 */
function commerce_paylinebymonext_params_payment($settings, $order, $contract_number) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.currencies');
  $payment = array(
    'mode' => $settings['mode'],
    'action' => $settings['action'],
    'amount' => $order->commerce_order_total[LANGUAGE_NONE][0]['amount'],
    'currency' => commerce_paylinebymonext_currency_format($order->commerce_order_total[LANGUAGE_NONE][0]['currency_code']),
    'contractNumber' => $contract_number,
  );
  return $payment;
}

/**
 * Return the payment params for WS call
 *
 * @param stdClass $order
 *  a commerce_order object
 * @return array
 */
function commerce_paylinebymonext_params_customer($order) {
  $billing = NULL;
  $shipping = NULL;
  $buyer = array();
  $billing_address = array();
  $shipping_address = array();
  if (property_exists($order, 'commerce_customer_billing')) {
    // Get customer billing informations from order
    $billing = commerce_customer_profile_load($order->commerce_customer_billing[LANGUAGE_NONE][0]['profile_id']);
    // Format customer billing informations for Payline WS
    $billing_address = commerce_paylinebymonext_params_address($billing);
    // Get buyer informations for Payline WS
    $buyer = commerce_paylinebymonext_params_buyer($billing, $order);
  }
  if (property_exists($order, 'commerce_customer_shipping')) {
    // Get customer shipping informations from order
    $shipping = commerce_customer_profile_load($order->commerce_customer_shipping[LANGUAGE_NONE][0]['profile_id']);
    // Format customer shipping informations for Payline WS
    $shipping_address = commerce_paylinebymonext_params_address($shipping);
    // If buyer informations is not available in b it billing informations get from shipping informations
    if (empty($buyer) || (!$buyer['lastName'] && !$buyer['firstName'])) {
      $buyer = commerce_paylinebymonext_params_buyer($shipping, $order);
    }
  }
  return array(
    'buyer' => $buyer,
    'billingAddress' => $billing_address,
    'shippingAddress' => $shipping_address,
  );
}

/**
 * Get buyer information for Payline WS call
 * from a commerce order and a commerce customer profile
 *
 * @param stdClass $profile
 *  a commerce_customer_profile object
 * @param stdClass $order
 *  a commerce_order object
 * @return array
 */
function commerce_paylinebymonext_params_buyer($profile, $order) {
  global $user;
  $customer_address = $profile->commerce_customer_address[LANGUAGE_NONE][0];
  $buyer = array(
    'firstName' => isset($customer_address['first_name']) ? $customer_address['first_name'] : NULL,
    'lastName' => isset($customer_address['last_name']) ? $customer_address['last_name'] : NULL,
    'ip' => isset($order->hostname) ? $order->hostname : NULL,
    'customerId' => $user->uid != 0 ? $user->uid : NULL,
    'email' => $user->uid != 0 ? $user->mail : NULL,
  );
  return $buyer;
}

/**
 * Format customer address information for Payline WS call
 * from a commerce customer profile
 *
 * @param stdClass $profile
 *  a commerce_customer_profile object
 * @return array
 */
function commerce_paylinebymonext_params_address($profile) {
  $customer_address = $profile->commerce_customer_address[LANGUAGE_NONE][0];
  $address = array(
    'name' => NULL,
    'street1' => isset($customer_address['thoroughfare']) ? $customer_address['thoroughfare'] : NULL,
    'street2' => isset($customer_address['premise']) ? $customer_address['premise'] : NULL,
    'cityName' => isset($customer_address['locality']) ? $customer_address['locality'] : NULL,
    'zipCode' => isset($customer_address['postal_code']) ? $customer_address['postal_code'] : NULL,
    'country' => isset($customer_address['country']) ? $customer_address['country'] : NULL,
  );
  return $address;
}

/**
 * Set order params for a doWebPayment request
 *
 * @param stdClass $order
 *  a commerce_order object
 * @param array $payment
 *  The already prepared payment params for the WS call
 * @return array
 */
function commerce_paylinebymonext_params_order($order, $payment) {
  // the tax component can be passed to payline service
  $tax = NULL;
  if (property_exists($order, 'commerce_order_total') && isset($order->commerce_order_total[LANGUAGE_NONE][0]['data']['components'])) {
    $price_components = $order->commerce_order_total[LANGUAGE_NONE][0]['data']['components'];
    foreach ($price_components as $component) {
      if (strpos($component['name'], 'tax|') !== FALSE) {
        if (!$tax) {
          $tax = $component['price']['amount'];
        }
        else {
          $tax += $component['price']['amount'];
        }
      }
    }
  }
  $order_param = array(
    'amount' => $payment['amount'],
    'currency' => $payment['currency'],
    'ref' => $order->order_id,
    'date' => isset($order->changed) ? format_date($order->changed, 'custom', 'd/m/Y H:i') : NULL,
    'taxes' => $tax,
  );
  return $order_param;
}

/**
 * the Payline interface url notification handler
 *  This function is the main controller on payline return event following offsite payment
 *  It do a getWebPaymentDetails to get the paylinie status of the corresponding transaction
 *  Launch the module commerce transaction handler process
 *  Manage the continuation of the commerce order process dependings the result of payline transaction
 *  And redirect the customer on the correct checkout step dependings the result of payline transaction
 *
 * @param string $method_id
 *  the used payment method id
 * @param stdClass $order
 *  a commerce_order object
 * @return boolean false
 */
function commerce_paylinebymonext_ppn($method_id, $order) {
  // get payline request params as we need the token to do the WS call
  $params = drupal_get_query_parameters();
  // load the full used payment method instance
  $payment_method = commerce_paylinebymonext_payment_method_load($method_id);
  // Set the commercePaylineSDK object to handle the WS call
  $payline = commerce_paylinebymonext_sdk($payment_method['settings']);
  // Get the payline transacition status and log the call WS process
  $response = $payline->getWebPaymentDetails(array('version' => 4, 'token' => $params['token']));
  $payline->saveLog();
  // ensure transaction correponds to the correct order.
  if ($order->order_id != $response['order']['ref']) {
    drupal_set_message(t("Store order and payline order ref doesn't match"), 'warning', FALSE);
    watchdog(COMMERCE_PAYLINE_WD,
      t('Payline transaction and Order does not match : attempt to use @token, corresponding to @transaction, to validate @order_id store order'),
      array('@token' => $params['token'], '@transaction' => $response['transaction']['id'], '@order_id' => $order->id),
      WATCHDOG_WARNING
    );
  } elseif (in_array($response['result']['code'], array('02306','02000','02005'))) {
    // if operation is already in progress, just log and return
    drupal_set_message(t("Operation already in progress"), 'warning', FALSE);
    watchdog(COMMERCE_PAYLINE_WD,
      t("Operation already in progress"),
      array(),
      WATCHDOG_NOTICE
    );
    return;
  }
  else {
    try {
      // if transaction corresponds to the correct order we process it in Drupal
      module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.transaction');
      $result = commerce_paylinebymonext_transaction_process($payment_method, $order, $response);
      // Manage the order status correponding to the payline transaction
      // and redirect user to the corresponding checkout page step
      if (!$result) {
        commerce_paylinebymonext_redirect_pane_previous_page($order);
      }
      else {
        commerce_paylinebymonext_redirect_pane_next_page($order);
      }
    } catch (Exception $e) {
      // if something goes wrong we inform customer that a unespected error happens, log the error,
      // manage the order status and redirect the user in the correct checkout step
      watchdog(COMMERCE_PAYLINE_WD, 'Payment return process uncountered unespectable error. @message / @trace', array(
        '@message' => $e->getMessage(),
        '@trace' => $e->getTraceAsString(),
      ), WATCHDOG_ERROR);
      commerce_paylinebymonext_set_warning($order);
      commerce_paylinebymonext_redirect_pane_previous_page($order);
    }
  }
  return FALSE;
}

/**
 * generic payline form for offsite methods
 *
 * @param $payment_method
 * @return array
 */
function commerce_paylinebymonext_checkout_offsite_form($payment_method) {
  // get the all mapping array contract types
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.contracts');
  $all_contracts = commerce_paylinebymonext_contracts_types();
  // alter settings to get primary contracts array
  $settings =& $payment_method['settings'];
  $settings['method_id'] = $payment_method['method_id'];
  drupal_alter('commerce_paylinebymonext_settings', $settings);
  // prepare form
  $form = array();
  // prepare options :
  $options = array();
  $default = NULL;
  foreach ($settings['primary_contracts'] as $contract_type => $info) {
    $default = $default ? $default : $info['contract_number'];
    $reformated_contract_type = commerce_paylinebymonext_contracts_reformat_type($contract_type);
    $img = commerce_paylinebymonext_contracts_prepare_picture($all_contracts[$reformated_contract_type]);
    $options[$info['contract_number']] = render($img);
  }
  $form['commerce_paylinebymonext_contract'] = array(
    '#type' => 'radios',
    '#options' => $options,
    '#default_value' => $default,
    '#attributes' => array(
      'class' => array('commerce-payline-contracts'),
    ),
  );
  if (count($options) > 1) {
    $form['commerce_paylinebymonext_contract']['#description'] = t('Please, choose a payment method');
  }
  // give opportunity to disable js behaviour in front office form
  if (variable_get('commerce_paylinebymonext_nojs', 0) == 0) {
    $form['#attached'] = array(
      'js' => array(
        drupal_get_path('module', 'commerce_paylinebymonext') . '/js/commerce_paylinebymonext.offsite.js',
      ),
    );
  }
  return $form;
}
