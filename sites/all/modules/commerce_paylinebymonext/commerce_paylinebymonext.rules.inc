<?php

/**
 * Implements hook_rules_condition_info().
 */
function commerce_paylinebymonext_rules_condition_info() {
  $conditions = array();

  $conditions['transaction_status_is'] = array(
    'label' => t('Transaction status is'),
    'parameter' => array(
      'commerce_payment_transaction' => array(
        'type' => 'commerce_payment_transaction',
        'label' => t('Transaction'),
      ),
      'operator' => array(
        'type' => 'text',
        'label' => t('Operator'),
        'default value' => 'equals',
        'option_list' => 'commerce_paylinebymonext_operator_options_list',
        'restriction' => 'input',
        'description' => t('The comparison operator'),
      ),
      'value' => array(
        'type' => 'text',
        'label' => t('Status'),
        'default value' => '',
        'options list' => 'commerce_payment_transaction_status_options_list',
        'description' => t('The value to compare against the transaction status'),
      ),
    ),
    'group' => t('Payline'),
    'callbacks' => array(
      'execute' => 'commerce_paylinebymonext_transaction_status_is',
    ),
  );

  return $conditions;
}

/**
 * @param $commerce_payment_transaction
 * @param $operator
 * @param $value
 */
function commerce_paylinebymonext_transaction_status_is($commerce_payment_transaction,$operator,$value) {
  $wrapper = entity_metadata_wrapper('commerce_payment_transaction',$commerce_payment_transaction);
  $val = $wrapper->status->value();
  if(is_object($commerce_payment_transaction)) {
    switch($operator) {
      case 'equals':
        if ($val == $value){
          $return = true;
        }
        else {
          $return = false;
        }
    }
  }
  return $return;
}

/**
 * Options list callback: operators for the transaction status comparison condition.
 */
function commerce_paylinebymonext_operator_options_list() {
  return array(
    'equals' => t('equals'),
  );
}

/**
 * Define the action: payline server is not accessible response code XXXXX (COMMERCE_PAYLINE_CODE_STATUS_GENERAL_FAILURE)
 * Implementation of hook_rules_event_info()
 */
function commerce_paylinebymonext_rules_event_info() {
  return array(
    'payline_connection_error' => array(
      'label' => t('Connection error with Payline server'),
      'module' => 'Payline',
      'arguments' => array(),
      'group' => t('Payline'),
    ),
  );
}
