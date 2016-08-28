<?php
/**
 * @file
 * commerce_paylinebymonext_sa_wp.rules_defaults.inc
 */

/**
 * Implements hook_default_rules_configuration().
 */
function commerce_paylinebymonext_sa_wp_default_rules_configuration() {
  //rules when order status is set to authorized
  $rule = rules_reaction_rule();
  $rule->label = 'Payline order authorized';
  $rule->active = TRUE;
  $rule->event('commerce_order_update')
    ->condition(rules_condition('data_is', array('data:select' => 'commerce-order:status', 'value' => 'completed')))
    ->condition(rules_condition('data_is', array('data:select' => 'commerce-order-unchanged:status', 'value' => 'authorized')))
    ->action('commerce_paylinebymonext_sa_wp_send_payment', array('commerce-order' => 'commerce-order'));

  $configs['commerce_paylinebymonext_sa_wp_authorized'] = $rule;

  //components
  $configs['commerce_paylinebymonext_sa_wp_transaction_completed'] = entity_import('rules_config','
    { "commerce_paylinebymonext_sa_wp_transaction_completed" : {
    "LABEL" : "Payline SAWP transaction complete",
    "PLUGIN" : "action set",
    "OWNER" : "rules",
    "TAGS" : [ "Payline" ],
    "REQUIRES" : [ "commerce_paylinebymonext_sa_wp" ],
    "USES VARIABLES" : {
      "commerce_payment_transaction" : { "label" : "Transaction", "type" : "commerce_payment_transaction" }
    },
    "ACTION SET" : [
      { "commerce_paylinebymonext_sa_wp_complete_transaction" : { "commerce_payment_transaction" : [ "commerce_payment_transaction" ] } }
    ]
  }
}'
  );

  return $configs;

}
