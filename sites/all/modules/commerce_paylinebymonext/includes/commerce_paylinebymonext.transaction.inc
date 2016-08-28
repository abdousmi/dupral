<?php

/**
 * @file
 *  Provide api function to handle commerce transaction creations.
 */

/**
 * Commerce payment transaction creation handler on payline payment transaction occuring
 *  On a Payline return payment a commerce_payment_transaction is created
 *
 * @param array $payment_method
 * @param stdClass $order
 * @param array $response
 *  the payline response from getWebPaymentDetails
 * @return boolean
 */
function commerce_paylinebymonext_transaction_process($payment_method, &$order, $response) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.currencies');
  $remote_id = $response['transaction']['id'];
  $message = array();
  $message_variable = array();
  $transaction = commerce_paylinebymonext_transaction_get($remote_id);
  if (!$transaction) {
    $transaction = commerce_payment_transaction_new($payment_method['method_id'], $order->order_id);
    if ($remote_id) {
      $transaction->remote_id = $remote_id;
    }
  }
  commerce_paylinebymonext_transaction_message_set($message, $message_variable, $response, $payment_method);
  // Give opportunity to add some message informations
  drupal_alter('commerce_paylinebymonext_transaction_message', $message, $message_variable, $response, $payment_method, $order);
  $transaction->message = implode('<br>', $message);
  $transaction->message_variables = $message_variable;
  $transaction->amount = $response['payment']['amount'];
  $transaction->currency_code = commerce_paylinebymonext_currency_format($response['payment']['currency'], 'nta');
  $transaction->payload = $response;
  $data = array(
    'payline_mode' => $response['payment']['mode'],
    'payline_action' => $response['payment']['action'],
    'payline_contract_number' => $response['payment']['contractNumber'],
    'refunded' => FALSE,
  );
  // Give opportunity to add data to transaction
  drupal_alter('commerce_paylinebymonext_transaction_data', $data, $response, $payment_method, $order);
  $transaction->data = $data;
  // we consider transaction as validated if payline return is
  // ok
  // ok with required manually check
  // transaction already
  if (in_array($response['result']['code'], array('00000','01001', '01913'))) {
    $result = commerce_paylinebymonext_transaction_valid_finalize($response['result']['code'], $order, $transaction);
  }
  // fraud suspected code
  //04003 fraud suspected but payment ok
  //04002 fraud suspected but payment ko
  elseif(in_array($response['result']['code'],array('COMMERCE_PAYLINE_CODE_STATUS_FRAUD_SUCCESS','COMMERCE_PAYLINE_CODE_STATUS_FRAUD_FAILURE'))) {
    $result = commerce_paylinebymonext_transaction_fraud_finalize($response['result']['code'], $order, $transaction);
  }
  elseif(in_array($response['result']['code'],array('COMMERCE_PAYLINE_CODE_STATUS_GENERAL_FAILURE'))) {
    rules_invoke_event('payline_connection_error');
    $result = commerce_paylinebymonext_transaction_unvalid_finalize($response['result']['code'], $order, $transaction);
  }
  else {
    $result = commerce_paylinebymonext_transaction_unvalid_finalize($response['result']['code'], $order, $transaction);
  }
  return $result;
}

/**
 * Set transaction status in success case
 *
 * @param int $code
 *  the payline result code
 * @param stdClass $order
 *  the commerce_order object
 * @param stdClass $transaction
 *  the commerce_payment_transaction object
 * @return boolean TRUE
 */
function commerce_paylinebymonext_transaction_valid_finalize($code, $order, $transaction) {
  $status = COMMERCE_PAYMENT_STATUS_SUCCESS;
  if ($code == '01001') {
    $status = COMMERCE_PAYMENT_STATUS_PENDING;
  }
  drupal_alter('commerce_paylinebymonext_transaction_status',$status,$order,$code);
  $transaction->status = $status;
  commerce_payment_transaction_save($transaction);
  return TRUE;
}

/**
 * Failure handler
 *  Set transaction failure status
 *  Set the error corresponding messages have to be displayed to the customer
 *
 * @param int $code
 *  the payline result code
 * @param stdClass $order
 *  the commerce_order object
 * @param stdClass $transaction
 *  the commerce_payment_transaction object
 * @return boolean FALSE
 */
function commerce_paylinebymonext_transaction_unvalid_finalize($code, $order, $transaction) {
  $status = COMMERCE_PAYMENT_STATUS_FAILURE;
  $prefix = substr($code, 0, 2);
  // the target variable store the error actor
  // @see commerce_paylinebymonext_transaction_target().
  $target = 'merchant';
  if ($prefix != '02') {
    $target = commerce_paylinebymonext_transaction_target($code);
  }
  if ($target == 'merchant' || !$target) {
    // Customer payline cancellation case
    if ($code == '02319') {
      drupal_set_message(t('Transaction cancelled by yourself'));
    }
    else {
      commerce_paylinebymonext_set_warning($order);
    }
  }
  // standard message if transaction was refused by the customer payment service
  else {
    drupal_set_message(t('We are sorry, your transaction has been refused.'));
  }
  $transaction->status = $status;
  commerce_payment_transaction_save($transaction);
  return FALSE;
}

/**
 *
 *  Fraud handler
 *  set transaction status to fraud suspected
 *
 * @param $code
 * @param $order
 * @param $transaction
 */
function commerce_paylinebymonext_transaction_fraud_finalize($code, $order, $transaction) {
  if($code == COMMERCE_PAYLINE_CODE_STATUS_FRAUD_FAILURE) {
    $status = COMMERCE_PAYLINE_TRANSACTION_STATUS_FRAUD_FAILURE;
    $transaction->status = $status;
    commerce_payment_transaction_save($transaction);
    return FALSE;
  }
  if($code == COMMERCE_PAYLINE_CODE_STATUS_FRAUD_SUCCESS) {
    $status = COMMERCE_PAYLINE_TRANSACTION_STATUS_FRAUD_SUCCESS;
    $transaction->status = $status;
    commerce_payment_transaction_save($transaction);
    return TRUE;
  }
}

/**
 * Set generic message to store in transaction object from the payline message details
 *
 * @param array $message
 * @param array $message_variable
 * @param array $response
 *  the payline response
 * @param array $payment_method
 *  the full loaded paymment method used to do the transaction
 */
function commerce_paylinebymonext_transaction_message_set(&$message, &$message_variable, $response, $payment_method) {
  $message = array(
    'Payline status : @shortMessage',
    'Payline message : @longMessage',
  );
  $message_variable = array(
    '@shortMessage' => $response['result']['shortMessage'],
    '@longMessage' => $response['result']['longMessage'],
  );
  if (isset($response['payment']['contractNumber']) && !empty($response['payment']['contractNumber'])) {
    module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.contracts');
    $contract = commerce_paylinebymonext_contracts_get_by_number($payment_method, $response['payment']['contractNumber']);
    array_unshift($message, 'Contract : @contract');
    if (isset($contract['name'])) {
      $message_variable['@contract'] = $contract['name'];
    } elseif ($response['transaction']['threeDSecure'] != 'N') {
      $message_variable['@contract'] = t('3D secure action');
    } else {
      $message_variable['@contract'] = t('No known contract');
    }
  }
  commerce_paylinebymonext_transaction_fraud($response, $message, $message_variable);
  commerce_paylinebymonext_transaction_cb_info($response, $message, $message_variable);
}

/**
 * Return refused transaction target
 *  Give information about the error actor when a transaction is refused
 *  to avoid to warn the customer about a refused transaction if the cause
 *  is not from his responsability.
 *
 * @param int $code
 *  the payline transaction result code
 * @return bool|string
 */
function commerce_paylinebymonext_transaction_target($code) {
  $target_mapping = commerce_paylinebymonext_transaction_target_mapping();
  foreach ($target_mapping as $target => $mapping) {
    if (in_array($code, $mapping)) {
      return $target;
    }
  }
  return FALSE;
}

/**
 * return refused transaction mapping between target / code
 *
 * @return array
 */
function commerce_paylinebymonext_transaction_target_mapping() {
  return array(
    'buyer' => array(
      '01100',
      '01101',
      '01103',
      '01108',
      '01111',
      '01113',
      '01117',
      '01118',
      '01119',
      '01120',
      '01129',
      '01130',
      '01200',
      '01201',
      '01202',
      '01206',
      '01208',
      '01209',
      '01915'
    ),
    'merchant' => array(
      '01109',
      '01110',
      '01114',
      '01115',
      '01116',
      '01121',
      '01122',
      '01123',
      '01125',
      '01126',
      '01127',
      '01128',
      '01180',
      '01181',
      '01182',
      '01183',
      '01184',
      '01197',
      '01198',
      '01199',
      '01207',
      '01280',
      '01902',
      '01904',
      '01907',
      '01909',
      '01912',
      '01913',
      '01914',
      '01917',
      '01940',
      '01941',
      '01942',
      '01943'
    ),
  );
}

/**
 * Return a transaction object from remote_id
 *
 * @param $remote_id
 * @return bool|stdClass
 */
function commerce_paylinebymonext_transaction_get($remote_id) {
  $transaction = FALSE;
  if ($remote_id) {
    $query = db_select('commerce_payment_transaction', 't')
      ->fields('t', array('transaction_id'))
      ->condition('t.remote_id', $remote_id)
      ->execute();
    $result = $query->fetchAssoc();
    if (!empty($result)) {
      $transaction = commerce_payment_transaction_load($result['transaction_id']);
    }
  }
  return $transaction;
}

/**
 * Set specific message in transaction message in exepted fraud case
 *
 * @param array $response
 * @param array $message
 * @param array $message_variable
 */
function commerce_paylinebymonext_transaction_fraud($response, &$message, &$message_variable) {
  if ($response['transaction']['isPossibleFraud']) {
    foreach (array('fraudResult', 'fraudResultDetails') as $info) {
      if (isset($response['transaction'][$info]) && !empty($response['transaction'][$info])) {
        $message[] = $info . ': @' . $info;
        $message_variable['@' . $info] = $response['transaction'][$info];
      }
    }
    array_unshift($message, 'Warning : Fraud suspected');
  }
}

/**
 * Set card information in transaction message
 *
 * @param array $response
 * @param array $message
 * @param array $message_variable
 */
function commerce_paylinebymonext_transaction_cb_info($response, &$message, &$message_variable) {
  if (isset($response['card']['number'])) {
    $message[] = 'Card type : @cardType';
    $message[] = 'Card number : @cardNumber';
    $message[] = 'Card expiration date : @expirationDate';
    $message_variable += array(
      '@cardType' => $response['card']['type'],
      '@cardNumber' => $response['card']['number'],
      '@expirationDate' => $response['card']['expirationDate'],
    );
  }
}
