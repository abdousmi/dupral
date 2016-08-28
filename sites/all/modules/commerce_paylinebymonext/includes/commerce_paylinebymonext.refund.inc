<?php

/**
 * @file
 *  Provide api function to handle refund features
 */

/**
 * Do the refund WS call
 *  note that produce a real credit demand to the customer account
 *  and debit the merchant account
 *
 * @param stdCladd $transaction
 *  the commerce_payment_transaction object corresponding to the transction to fully or partialy refund
 * @param array $payment_method
 *  the full loaded payment method instance
 * @param int $amount
 *  the amount to refund
 * @return array
 *  The payline response from the refund WS call
 */
function commerce_paylinebymonext_refund_call($transaction, $payment_method, $amount) {
  $payline = commerce_paylinebymonext_sdk($payment_method['settings']);
  $params = commerce_paylinebymonext_refund_params($transaction, $amount);
  drupal_alter('commerce_paylinebymonext_params', $params);
  $response = $payline->doRefund($params);
  $payline->saveLog();
  return $response;
}

/**
 * Set the refund params for the WS call
 *
 * @param stdCladd $transaction
 *  the commerce_payment_transaction object corresponding to the transction to fully or partialy refund
 * @param int $amount
 *  the amount to refund
 * @return array
 */
function commerce_paylinebymonext_refund_params($transaction, $amount) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.currencies');
  $params = array(
    'version' => 4,
    'transactionID' => $transaction->remote_id,
    'privateDataList' => NULL,
    'comment' => NULL,
    'sequenceNumber' => NULL,
    'payment' => array(
      'amount' => $amount,
      'currency' => commerce_paylinebymonext_currency_format($transaction->currency_code),
      'action' => 421,
      'mode' => $transaction->data['payline_mode'],
      'contractNumber' => $transaction->data['payline_contract_number'],
    ),
  );
  return $params;
}

/**
 * @param int $amount
 *  the amount to refund
 * @param array $response
 *  the payline refund WS response
 * @param object stdCladd $transaction
 *  the commerce_payment_transaction object corresponding to the transction to fully or partialy refund
 * @param object stdCladd $order
 *  the commerce_order object corresponding to the transaction to refund
 * @param array $payment_method
 *  the full loaded payment method instance
 * @param string $new_status
 *  the order status after refund done (if success)
 * @param string $status_original
 *  the original order status before refund was attemped
 * @param string $log
 *  order documemtation for refund
 * @param array $line_items
 *  containing the commerce_line_items objects removed and informations (quantity, full removing) during the refund process
 */
function commerce_paylinebymonext_refund_process($amount, $response, $transaction, $order, $payment_method, $new_status = NULL, $status_original = NULL, $log = '', $line_items) {
  try {
    // Save the refund transaction
    if ($response['result']['code'] == '00000') {
      // if success
      payline_refund_transaction_process('valid', $amount, $response, $transaction, $payment_method, $order);
      // if order has to be updated
      if ($new_status != $status_original || !empty($line_items)) {
        commerce_paylinebymonext_refund_order_update($order, $new_status, $status_original, $line_items, $log);
      }
    }
    else {
      // if failure
      payline_refund_transaction_process('unvalid', $amount, $response, $transaction, $payment_method, $order);
    }
  } catch (Exception $e) {
    // if error happens inform the merchant and log it
    watchdog(COMMERCE_PAYLINE_WD, 'Refund return process uncountered unespectable error', array(
      '@message' => $e->getMessage(),
      '@trace' => $e->getTraceAsString(),
    ), WATCHDOG_ERROR);
    drupal_set_message(t('Refund return process uncountered unexpected error'), 'warning', FALSE);
  }
}

/**
 * Commerce payment transaction handler for the refund
 *
 * @param string $status
 *  the refund status, value can be
 *   * valid
 *   * unvalid
 * @param int $amount
 *  amount refunded
 * @param array $response
 *  the payline response
 * @param sdtClass $transaction
 *  the commerce_payment_transaction object has been refunded
 * @param array $payment_method
 *  the full loaded payment method used in the transcation has been refunded
 * @param stdClass $order
 *  the commerce_order object correponding to the transaction has been refunded
 */
function payline_refund_transaction_process($status, $amount, $response, $transaction, $payment_method, $order) {
  $function = 'commerce_paylinebymonext_refund_transaction_' . $status . '_set';
  $message = array();
  $message_variables = array();
  $data = array();
  commerce_paylinebymonext_refund_transaction_prepare($message, $message_variables, $data, $response, $payment_method, $transaction);
  $transaction_to_save = $function($amount, $response, $message, $message_variables, $data, $transaction, $order, $payment_method);
  commerce_paylinebymonext_refund_transaction_finalize($transaction_to_save, $message, $message_variables, $data, $payment_method, $response, $order);
}

/**
 * Prepare data and message for the refund transaction
 *
 * @param array $message
 * @param array $message_variables
 * @param array $data
 * @param array $response
 *  the payline response
 * @param array $payment_method
 *  the full loaded payment method used in the transcation has been refunded
 * @param sdtClass $transaction
 *  the commerce_payment_transaction object has been refunded
 */
function commerce_paylinebymonext_refund_transaction_prepare(&$message, &$message_variables, &$data, $response, $payment_method, $transaction) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.transaction');
  commerce_paylinebymonext_transaction_message_set($message, $message_variables, $response, $payment_method);
  $data = array(
    'payline_mode' => $transaction->data['payline_mode'],
    'payline_action' => 421,
    'payline_contract_number' => $transaction->data['payline_contract_number'],
    'payline_transaction_refunded' => $transaction->transaction_id,
  );
}

/**
 * Refund transaction create handler in success case
 *
 * @param array $response
 *  the payline response
 * @param int $amount
 *  the refunded amount
 * @param array $message
 * @param array $message_variables
 * @param $data
 * @param sdtClass $transaction
 *  the commerce_payment_transaction object has been refunded
 * @param stdClass $order
 *  the commerce_order object correponding to the transaction has been refunded
 * @param array $payment_method
 *  the full loaded payment method used in the transcation has been refunded
 * @return stdClass
 *  A new commerce_payment_transaction object.
 */
function commerce_paylinebymonext_refund_transaction_valid_set($amount, $response, &$message, &$message_variables, &$data, $transaction, $order, $payment_method) {
  // Update the original transaction
  commerce_paylinebymonext_refund_transaction_payment_update($transaction, $amount);
  // The refund transaction is stored with the payment method corresponding to the refunded transaction
  $method_id = $transaction->payment_method;
  $transaction_to_save = commerce_payment_transaction_new($method_id, $order->order_id);
  $transaction_to_save->currency_code = $transaction->currency_code;
  $data['payline_transaction_refunded'] = $transaction->transaction_id;
  array_unshift($message, 'Refund of @remote_id (payline) / @transaction_id (drupal commerce)', 'Refund of @refund_amount');
  $message_variables['@refund_amount'] = commerce_currency_format($amount, $transaction->currency_code);
  $message_variables['@remote_id'] = $transaction->remote_id;
  $message_variables['@transaction_id'] = $transaction->transaction_id;
  $transaction_to_save->amount = -$amount;
  $transaction_to_save->status = COMMERCE_PAYLINE_TRANSACTION_STATUS_REFUND;
  return $transaction_to_save;
}

/**
 * Refund transaction create handler in success case
 *
 * @param array $response
 *  the payline response
 * @param int $amount
 *  the refunded amount
 * @param array $message
 * @param array $message_variables
 * @param $data
 * @param sdtClass $transaction
 *  the commerce_payment_transaction object has been refunded
 * @param stdClass $order
 *  the commerce_order object correponding to the transaction has been refunded
 * @param array $payment_method
 *  the full loaded payment method used in the transcation has been refunded
 * @return stdClass
 *  A new commerce_payment_transaction object.
 */
function commerce_paylinebymonext_refund_transaction_unvalid_set($amount, $response, &$message, &$message_variables, &$data, $transaction, $order, $payment_method) {
  $transaction_to_save = commerce_paylinebymonext_transaction_get($response['transaction']['id']);
  if (!$transaction_to_save) {
    $transaction_to_save = commerce_payment_transaction_new($payment_method['method_id'], $order->order_id);
  }
  $transaction_to_save->amount = -$amount;
  $transaction_to_save->currency_code = $transaction->currency_code;
  $transaction_to_save->status = COMMERCE_PAYLINE_TRANSACTION_STATUS_REFUND_FAILURE;
  return $transaction_to_save;
}

/**
 * @param stdClass $transaction
 *  the commerce_payment_transaction corresponds to the new refund transaction
 * @param array $message
 * @param array $message_variables
 * @param array $data
 * @param array $payment_method
 *  the full loaded payment method used in the transcation has been refunded
 * @param array $response
 *  the payline response
 * @param stdClass $order
 *  the commerce_order object correponding to the transaction has been refunded
 */
function commerce_paylinebymonext_refund_transaction_finalize($transaction, $message, $message_variables, $data, $payment_method, $response, $order) {
  if ($response['transaction']['id']) {
    $transaction->remote_id = $response['transaction']['id'];
  }
  // give opportunity to add message information
  drupal_alter('commerce_paylinebymonext_transaction_message', $message, $message_variables, $response, $payment_method, $order);
  $transaction->message = implode('<br>', $message);
  $transaction->message_variables = $message_variables;
  $transaction->payload = $response;
  // give opportunity to add data to new transaction
  drupal_alter('commerce_paylinebymonext_transaction_data', $data, $response, $payment_method, $order);
  $transaction->data = $data;
  commerce_payment_transaction_save($transaction);
}

/**
 * Update an order after a success refund transaction
 *
 * @param stdClass $order
 *  the commerce_order object to update
 * @param string $new_status
 *  the order status after refund done (if success)
 * @param string $status_original
 *  the original order status before refund was attemped
 * @param array $line_items
 *  containing the commerce_line_items objects removed and informations (quantity, full removing) during the refund process
 * @param string $log
 *  order documemtation for refund
 * @return stdClass
 *  the updated_commerce_order object
 */
function commerce_paylinebymonext_refund_order_update($order, $new_status, $status_original, $line_items, $log) {
  $order_to_update = 0;
  // update line items if change has occured during refund process
  if (!empty($line_items)) {
    foreach ($line_items as $line_item_id => $item) {
      if ($item['remove']) {
        commerce_line_item_delete($line_item_id);
      }
      else {
        $original_quantity = (int) ($item['line_item']->quantity * 100);
        $new_quantity = (int) ($item['quantity'] * 100);
        if ($new_quantity < $original_quantity) {
          // Update the line item based on the values in the additional elements.
          $line_item = clone($item['line_item']);
          $line_item->quantity = $item['quantity'];
          commerce_line_item_save($line_item);
          $order_to_update = 1;
        }
      }
    }
  }
  // update order status
  if ($new_status != $status_original) {
    commerce_order_status_update($order, $new_status, FALSE, NULL, $log);
  }
  elseif ($order_to_update) {
    $order = commerce_order_save($order);
  }
  return $order;
}

/**
 * Update a existing payment transaction on refund
 *  Set it as refunded
 *  Set refunde_amount data
 *  Set new information messages
 *
 * @param stdClass $transaction
 *  The commerce_payment_transaction refunded
 * @param int $amount
 *  The refunded amount
 */
function commerce_paylinebymonext_refund_transaction_payment_update(&$transaction, $amount) {
  if ($transaction->data['refunded']) {
    $transaction->data['refund_amount'] += $amount;
  }
  else {
    $transaction->data['refunded'] = TRUE;
    $transaction->data['refund_amount'] = $amount;
  }
  $message = explode('<br>', $transaction->message);
  $message[] = '@refundedAmount have been refunded of this transaction';
  $message_variables = $transaction->message_variables;
  $message_variables['@refundedAmount'] = commerce_currency_format($transaction->data['refund_amount'], $transaction->currency_code);
  $transaction->message = implode($message);
  $transaction->message_variables = $message_variables;
  commerce_payment_transaction_save($transaction);
}