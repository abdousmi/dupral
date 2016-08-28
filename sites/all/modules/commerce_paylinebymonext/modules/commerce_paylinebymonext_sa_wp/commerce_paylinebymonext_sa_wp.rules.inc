<?php
/**
 * @file
 * commerce_paylinebymonext_sa_wp.rules.inc
 */

/**
 * Implement hook_action_info to custom action
 *
 */
function commerce_paylinebymonext_sa_wp_rules_action_info() {

  $actions = array();

  $actions['commerce_paylinebymonext_sa_wp_send_payment'] = array(
    'label' => t('Payline capture payment: send payment to payline'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Order in checkout'),
      ),
    ),
    'group' => t('Payline'),
    'callbacks' => array(
      'execute' => 'commerce_paylinebymonext_sa_wp_send_payment',
    ),
  );

  $actions['commerce_paylinebymonext_sa_wp_complete_transaction'] = array(
    'label' => t('Payline set transaction complete'),
    'parameter' => array(
      'commerce_payment_transaction' => array(
        'type' => 'commerce_payment_transaction',
        'label' => t('Transaction'),
      ),
    ),
    'group' => t('Payline'),
    'callbacks' => array(
      'execute' => 'commerce_paylinebymonext_sa_wp_complete_transaction',
    ),
  );
  return $actions;
}

/**
 *
 * Callback function for commerce_paylinebymonext_sa_wp_send_payment rule
 * It sends the payment by making a doCapture request or a doReAutorisation request(if first authorisation is older than
 * a week.
 * The order status : from AUTHORIZED to COMPLETED or PAYMENT_FAILED
 * The transaction payment status: from PAYLINE_AUTHORIZED to SUCESS
 *
 * @param $order
 */
function commerce_paylinebymonext_sa_wp_send_payment($order) {

  $mustReAuthorize = false;

  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.currencies');
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.transaction');

  // Getting transaction for that order
  $payments = commerce_payment_transaction_load_multiple(array(), array('order_id' =>  $order->order_id, 'status' => 'payline_authorized'));
  if(count($payments) != 1) {
    if(count($payments) > 1){
      drupal_set_message(t('Too much payment transaction.'), 'warning', FALSE);
      watchdog(COMMERCE_PAYLINE_WD, 'Too much payment transaction.', WATCHDOG_ERROR);
    }
    if(count($payments) < 1){
      drupal_set_message(t('No authorisation transaction to validate.'), 'warning', FALSE);
      watchdog(COMMERCE_PAYLINE_WD, 'No authorisation transaction to validate.', WATCHDOG_ERROR);
    }
    return;
  }
  // Get only one transaction
  $payment = !empty($payments) ? array_shift($payments) : NULL;

  //is the authorization older than a week
  if($payment->created < strtotime("-1 week")) {
    $mustReAuthorize = true;
  }

  //get amount and currency of the order
  $wrapper = entity_metadata_wrapper('commerce_order', $order);
  $total = $wrapper->commerce_order_total->amount->value();
  $currency_code = $wrapper->commerce_order_total->currency_code->value();

  //get the contract number
  $contractNumber = $payment->data['payline_contract_number'];
  //get the method id
  $method_id = $order->data['payment_method'];

  //Construct the params array
  $params = array();
  $params['version'] = '4';
  $params['transactionID'] = $payment->remote_id;
  $params['payment'] = array(
    'amount' => $total,
    'currency' => commerce_paylinebymonext_currency_format($currency_code),
    'action' => '201',
    'mode' => 'CPT',
    'contractNumber' => $contractNumber,
  );

  if($mustReAuthorize) {
    $params['payment']['action'] = 101;
    $params['order'] = array(
      'ref' => $payment->order_id,
      'amount' => $payment->amount,
      'currency' => commerce_paylinebymonext_currency_format($payment->currency_code),
      'date' => date('d/m/Y h:i', $payment->created),
    );
  }
  else{
    $params['sequenceNumber'] = '';
  }

  // load the full used payment method instance
  $payment_method = commerce_paylinebymonext_payment_method_load($method_id);

  // Set the commercePaylineSDK object to handle the WS call
  $payline = commerce_paylinebymonext_sdk($payment_method['settings']);

  try{
    if($mustReAuthorize){
      // Get the payline transacition status and log the call WS process
      $response = $payline->doReAuthorization($params);
    }
    else{
      // Get the payline transacition status and log the call WS process
      $response = $payline->doCapture($params);
    }

    if($response['result']['code'] == '00000' ){
      //set transaction status to success
      rules_invoke_component('commerce_paylinebymonext_sa_wp_transaction_completed',$payment);
    }

    else{
      if($mustReAuthorize) {
        throw new Exception('Payline doReAuthorization response code: '.$response['result']['code']);
      }
      else {
        throw new Exception('Payline doCapture response code: '.$response['result']['code']);
      }

    }
  } catch (Exception $e) {
    // if something goes wrong we inform customer that a unespected error happens, log the error,
    // manage the order status and redirect the user in the correct checkout step
    watchdog(COMMERCE_PAYLINE_WD, 'Payment return process uncountered unespectable error. @message / @trace', array(
      '@message' => $e->getMessage(),
      '@trace' => $e->getTraceAsString(),
    ), WATCHDOG_ERROR);
    commerce_paylinebymonext_set_warning($order);
  }
}

/**
 *
 * Callback function set transaction status to success
 *
 * @param $commerce_payment_transaction
 */
function commerce_paylinebymonext_sa_wp_complete_transaction($commerce_payment_transaction) {
  $commerce_payment_transaction->status = 'success';
  entity_save('commerce_payment_transaction',$commerce_payment_transaction);
}
