<?php

/**
 * @file
 *  Provide back office form function (exept payment methods form settings)
 */

/**
 * Refund page callback function
 *
 * @param stdClass $order
 *  a commerce_order object
 * @param stdClass $transaction
 *  a commerce_payment_transaction object
 * @return string
 *  html form output
 */
function commerce_paylinebymonext_ui_transaction_refund_form_wrapper($order, $transaction) {
  drupal_set_title(t('Refund transaction @transaction_id for order @order_id', array('@transaction_id' => $transaction->remote_id, '@order_id' => $order->order_id)));
  $form = drupal_get_form('commerce_paylinebymonext_ui_transaction_refund_form', $order, $transaction);
  return render($form);
}

/**
 * Form function for transaction refund confirm
 *
 * @param array $form
 * @param array $form_state
 * @param stdClass $order
 *  a commerce_order object
 * @param stdClass $transaction
 *  a commerce_payment_transaction object
 * @return array
 *  a renderable form array
 */
function commerce_paylinebymonext_ui_transaction_refund_form($form, &$form_state, $order, $transaction) {
  // Ensure this include file is loaded when the form is rebuilt from the cache.
  $form_state['build_info']['files']['form'] = drupal_get_path('module', 'commerce_order') . '/includes/commerce_order.forms.inc';
  $form = array();
  $form['#attached']['css'] = array(
    drupal_get_path('module', 'commerce_paylinebymonext_ui') . '/css/commerce_paylinebymonext_ui_refund.css',
  );
  // Add the field related form elements.
  $form_state['commerce_order'] = $order;
  field_attach_form('commerce_order', $order, $form, $form_state);
  // Hide the order total field from direct editing.
  $form['commerce_order_total']['#access'] = FALSE;
  // unset unused fields
  unset($form['commerce_line_items'][$form['commerce_line_items']['#language']]['actions']);
  unset($form['commerce_line_items'][$form['commerce_line_items']['#language']]['#element_validate']);
  unset($form['commerce_customer_billing']);
  unset($form['commerce_customer_shipping']);
  unset($form['commerce_discounts']);
  // don't give opportunity to change price of line item
  foreach ($form['commerce_line_items'][$form['commerce_line_items']['#language']]['line_items'] as $line_item_id => &$line_item_form) {
    if (is_numeric($line_item_id)) {
      $line_item_form['commerce_unit_price']['#disabled'] = TRUE;
    }
  }
  // Change the line items manager title
  $form['commerce_line_items'][$form['commerce_line_items']['#language']]['#title'] = t('Items manager');

  // Refund operation fieldsed
  $form['refund_operation'] = array(
    '#type' => 'fieldset',
    '#title' => t('Refund Type'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  // checkbox element to choose refund without remove items or not
  $form['refund_operation']['refund_with_all'] = array(
    '#type' => 'checkbox',
    '#title' => t('Refund without remove items'),
    '#description' => t('Do you want perform a refund without remove any items ?'),
    '#weight' => -12,
    '#ajax' => array(
      'callback' => 'commerce_paylinebymonext_ui_refund_without_remove_js',
      'wrapper' => 'edit-commerce-line-items',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );
  // replace the line item manager in the refund operation fieldset
  $form['refund_operation']['commerce_line_items'] = $form['commerce_line_items'];
  unset($form['commerce_line_items']);
  // free amount field
  // access set to false by default as we push refund by line items
  $form['refund_operation']['commerce_line_items']['refund_amount'] = array(
    '#title' => t('Amount to refund'),
    '#type' => 'textfield',
    '#size' => '10',
    '#default_value' => number_format($transaction->amount / 100, 2),
    '#access' => FALSE,
  );
  // ensure the free amount value filled in form will be concidered on a submit action
  // we have to do that has on submit the form is rebuild and so on, even if the access property
  // was set to true during the ajax process it will be set to false again on rebuild
  if (isset($form_state['values']['refund_with_all']) && $form_state['values']['refund_with_all']) {
    $form['refund_operation']['commerce_line_items']['refund_amount']['#access'] = TRUE;
  }
  // Set the disabled currency field (only for information)
  $form['refund_operation']['commerce_line_items']['refund_currency'] = array(
    '#type' => 'select',
    '#options' => array($transaction->currency_code => $transaction->currency_code),
    '#default_value' => $transaction->currency_code,
    '#access' => FALSE,
    '#disabled' => TRUE,
  );
  // Description of which refund operations can be done
  $form['refund_operation']['commerce_line_items']['refund_description'] = array(
    '#theme' => 'html_tag',
    '#tag' => 'div',
    '#attributes' => array('class' => array('refund-without-remove-description', 'description')),
    '#value' => t('Remove items you want to refund (check remove checkbox or decrease quantity).'),
  );
  // Build an array of order status options grouped by order state.
  $options = array();
  foreach (commerce_order_state_get_title() as $name => $title) {
    foreach (commerce_order_statuses(array('state' => $name)) as $order_status) {
      $options[check_plain($title)][$order_status['name']] = check_plain($order_status['title']);
    }
  }
  // Add a section to update the status and leave a log message.
  $form['order_status'] = array(
    '#type' => 'fieldset',
    '#title' => t('Order status'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['order_status']['status'] = array(
    '#type' => 'select',
    '#title' => t('Status after refund'),
    '#options' => $options,
    '#default_value' => $order->status,
    '#description' => t('Which status order will be get afer you perform refund'),
  );
  $form['order_status']['status_original'] = array(
    '#type' => 'hidden',
    '#value' => $order->status,
    '#attributes' => array('id' => 'edit-status-original'),
  );
  $form['order_status']['log'] = array(
    '#type' => 'textarea',
    '#title' => t('Update log message'),
    '#description' => t('Provide an explanation of the changes you are making. This will provide a meaningful audit trail for updates to this order.'),
    '#rows' => 4,
  );
  // Prepare the confirm form
  $question = t('Confirm transaction refund (payline transaction : @remote_id) for order @order_id', array('@remote_id' => $transaction->remote_id, '@order_id' => $order->order_id));
  $path = 'admin/commerce/orders/' . $order->order_id . '/payment';
  $description = t('Are you sure to want to refund a part of this order ?');
  $form = confirm_form($form, check_plain($question), $path, check_plain($description), t('Refund'));
  return $form;
}

/**
 * Ajax callback function for the refund with all checkbox.
 *
 * @param array $form
 * @param array $form_state
 * @return array
 *  a renderable array of form elements part
 */
function commerce_paylinebymonext_ui_refund_without_remove_js(&$form, $form_state) {
  if ($form_state['values']['refund_with_all'] == 1) {
    $form['refund_operation']['commerce_line_items'][$form['refund_operation']['commerce_line_items']['#language']]['#access'] = FALSE;
    $form['refund_operation']['commerce_line_items']['refund_amount']['#access'] = TRUE;
    $form['refund_operation']['commerce_line_items']['refund_currency']['#access'] = TRUE;
    $form['refund_operation']['commerce_line_items']['refund_description']['#value'] = t('Fill the amount you want to refund for this order - any line item will be removed.');
  }
  // reset the id attribute of the wrapper to avoid incrementation and ajax process failure
  $form['refund_operation']['commerce_line_items']['#attributes']['id'] = 'edit-commerce-line-items';
  return $form['refund_operation']['commerce_line_items'];
}

/**
 * Refund form validate function
 *
 * @param array $form
 * @param array $form_state
 */
function commerce_paylinebymonext_ui_transaction_refund_form_validate(&$form, &$form_state) {
  $order = $form_state['commerce_order'];
  $wrapper = entity_metadata_wrapper('commerce_order', $order);
  // If free amount refund operation, ensure that refunded amouunt doesn't exced original transaction amount
  if ($form_state['values']['refund_with_all']) {
    $original_amount = $wrapper->commerce_order_total->amount->value();
    $refunded_amount = $form_state['values']['commerce_line_items']['refund_amount'] * 100;
    // If error is detected, reset the free amount fields access to true and the line items manager access to false
    // to get the correct display
    if ((int) $refunded_amount > (int) $original_amount) {
      $form['refund_operation']['commerce_line_items']['refund_amount']['#access'] = TRUE;
      $form['refund_operation']['commerce_line_items']['refund_currency']['#access'] = TRUE;
      $form['refund_operation']['refund_with_all']['#default_value'] = 1;
      $form['refund_operation']['commerce_line_items'][$form['refund_operation']['commerce_line_items']['#language']]['#access'] = FALSE;
      form_error($form['refund_operation']['commerce_line_items']['refund_amount'], t('Amount to refund is greater than orginal amount'));
    }
  }
  // if line items refund, ensure line items quantities doesn't exced the original quantities
  else {
    $line_items = $form_state['values']['commerce_line_items'][LANGUAGE_NONE]['line_items'];
    $flag = 0;
    foreach ($line_items as $item_id => $item) {
      $original_quantity = (int) ($item['line_item']->quantity * 100);
      $new_quantity = (int) ($item['quantity'] * 100);
      if ($new_quantity > $original_quantity && $item['remove'] !== 1) {
        $flag = 1;
        form_error($form['refund_operation']['commerce_line_items'][$form['refund_operation']['commerce_line_items']['#language']]['line_items'][$item_id]['quantity'],
          t('Qantity of product is greater than original quantity'));
      }
    }
    // if no error on quantities, ensure a refund operation has been filled
    // we have to do that as any refund operation are mandatory as multiple options
    // are provided
    if ($flag == 0) {
      $refunded_amount = commerce_paylinebymonext_ui_amount_to_refund($form_state);
      if ((int) $refunded_amount === 0) {
        form_error($form['refund_operation'], t('You have perform no refund operation'));
      }
    }
  }
}

/**
 * Refund form submit function
 *
 * @param array $form
 * @param array $form_state
 */
function commerce_paylinebymonext_ui_transaction_refund_form_submit(&$form, &$form_state) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.refund');
  $line_items = array();
  // If refund by line items maneger was done, set the line items information array
  // to be passed to the refund process handler
  if ($form_state['values']['refund_with_all'] == 0) {
    $line_items = $form_state['values']['commerce_line_items'][LANGUAGE_NONE]['line_items'];
  }
  // get other params to be passed to the refund process handler
  $order = $form_state['build_info']['args'][0];
  $transaction = $form_state['build_info']['args'][1];
  $method_id = $transaction->payment_method;
  $payment_method = commerce_paylinebymonext_payment_method_load($method_id);
  $amount = commerce_paylinebymonext_ui_amount_to_refund($form_state);
  // Request Payline Server for refund process
  $response = commerce_paylinebymonext_refund_call($transaction, $payment_method, $amount);
  // Process refund transaction and order management
  commerce_paylinebymonext_refund_process($amount, $response, $transaction, $order, $payment_method, $form_state['values']['status'], $form_state['values']['status_original'], $form_state['values']['log'], $line_items);
  // redirect merchant on order transactions list
  $form_state['redirect'] = 'admin/commerce/orders/' . $order->order_id . '/payment/';
}

/**
 * Get the amount to be refund from refund operation
 *
 * @param array $form_state
 * @return int
 */
function commerce_paylinebymonext_ui_amount_to_refund($form_state) {
  $amount = 0;
  // If free amount just done the *100 operation to get a int amount
  if ($form_state['values']['refund_with_all']) {
    $amount += $form_state['values']['commerce_line_items']['refund_amount'] * 100;
  }
  // Get the amount of line items quantity removed from order
  else {
    $line_items = $form_state['values']['commerce_line_items'][LANGUAGE_NONE]['line_items'];
    foreach ($line_items as $item_id => $item) {
      if ($item['remove'] == 1) {
        $quantity = (float) $item['line_item']->quantity;
      }
      else {
        $quantity = (float) $item['line_item']->quantity - (float) $item['quantity'];
      }
      $amount += $quantity * $item['commerce_unit_price'][LANGUAGE_NONE][0]['amount'];
    }
  }
  return (int)$amount;
}