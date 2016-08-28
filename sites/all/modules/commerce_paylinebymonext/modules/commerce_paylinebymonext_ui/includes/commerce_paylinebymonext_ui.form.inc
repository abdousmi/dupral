<?php

/**
 * @file
 *  Provide generic form function for Payline payment methods settings form
 */

/**
 * Standard payline payment method settings form
 *
 * @param array $settings
 * @return array
 */
function commerce_paylinebymonext_ui_settings_form($settings) {
  module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.contracts');
  drupal_alter('commerce_paylinebymonext_settings', $settings);
  $sellPoints = array(-1 => '------------');
  $form = array();
  $form['#attached']['css'][] = drupal_get_path('module', 'commerce_paylinebymonext') . '/css/commerce_paylinebymonext.settings.css';
  $form['#validate'][] = 'commerce_paylinebymonext_ui_settings_form_validate';
  $form['#submit'][] = 'commerce_paylinebymonext_ui_settings_form_submit';
  $form['access'] = array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#title' => t('Payline Global Access'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Set your payline global access (merchant id, access key and target environnement'),
    'merchant_id' => array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#title' => t('Merchand ID'),
      '#description' => t('Your payline merchant id'),
      '#default_value' => isset($settings['access']['merchant_id']) ? $settings['access']['merchant_id'] : '',
    ),
    'access_key' => array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#title' => t('Access Key'),
      '#description' => t('Your payline access key'),
      '#default_value' => isset($settings['access']['access_key']) ? $settings['access']['access_key'] : '',
    ),
    'production' => array(
      '#type' => 'radios',
      '#required' => TRUE,
      '#title' => t('Target environnement'),
      '#options' => array(
        0 => t('Homologation'),
        1 => t('Production'),
      ),
      '#description' => t('Your payline environnement'),
      '#default_value' => isset($settings['access']['production']) ? $settings['access']['production'] : 0,
    ),
  );

  $form['proxy'] = array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#title' => t('Proxy'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('Set your proxy environnement'),
    'proxy_host' => array(
      '#type' => 'textfield',
      '#title' => t('Proxy Host'),
      '#description' => t('Your proxy host'),
      '#default_value' => isset($settings['proxy']['proxy_host']) ? $settings['proxy']['proxy_host'] : '',
      '#weight' => '0',
    ),
    'proxy_port' => array(
      '#type' => 'textfield',
      '#title' => t('Proxy Port'),
      '#description' => t('Your proxy port'),
      '#default_value' => isset($settings['proxy']['proxy_port']) ? $settings['proxy']['proxy_port'] : '',
      '#weight' => '1',
    ),
    'proxy_user' => array(
      '#type' => 'textfield',
      '#title' => t('Proxy User'),
      '#description' => t('Your proxy user'),
      '#default_value' => isset($settings['proxy']['proxy_user']) ? $settings['proxy']['proxy_user'] : '',
      '#weight' => '2',
    ),
    'proxy_pwd' => array(
      '#type' => 'textfield',
      '#title' => t('Proxy Password'),
      '#description' => t('Your proxy password'),
      '#default_value' => isset($settings['proxy']['proxy_pwd']) ? $settings['proxy']['proxy_pwd'] : '',
      '#weight' => '3',
    ),
  );

  $form['contracts'] = array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#title' => t('Contracts'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Choose your primary and secundary payline contracts'),
    '#access' => FALSE,
  );
  $form['contracts']['sellpoint'] = array(
    '#tree' => TRUE,
    '#type' => 'select',
    '#title' => t('Payline Sell Point'),
    '#description' => t('Select the Payline sell point you want to use.'),
    '#options' => $sellPoints,
    '#default_value' => isset($settings['contracts']['sellpoint']) ? $settings['contracts']['sellpoint'] : -1,
    '#ajax' => array(
      'callback' => 'commerce_paylinebymonext_ui_form_reformat',
      'wrapper' => 'edit-parameter-payment-method',
      'method' => 'replace',
      'effect' => 'fade',
      'event' => 'change'
    ),
  );

  // prepare contracts list form elements
  if (isset($settings['contracts']['sellpoints'])) {
    $contracts_base_info = commerce_paylinebymonext_contracts_types();
    foreach ($settings['contracts']['sellpoints'] as $key => $sellpoints_contract) {
      foreach ($sellpoints_contract as $type => $infos) {
        $logo_type = commerce_paylinebymonext_contracts_reformat_type($type);
        $img = array(
          '#theme' => 'image',
          '#title' => $settings['contracts']['sellpoints'][$key][$type]['label'],
          '#path' => $contracts_base_info[$logo_type]['path'],
          '#alt' => t('@label logo', array('@label' => $settings['contracts']['sellpoints'][$key][$type]['label'])),
        );

        $form['contracts']['sellpoints'][$key][$type] = array(
          '#type' => 'fieldset',
          '#access' => FALSE,
          '#attributes' => array('class' => array('payline-contract-item')),
          'contract_logo' => array(
            $img,
          ),
          'information' => array(
            '#theme' => 'html_tag',
            '#tag' => 'div',
            '#attributes' => array(
              'class' => array('payline-contract-information'),
            ),
            '#value' => t('@label contract', array('@label' => $settings['contracts']['sellpoints'][$key][$type]['label'])),
          ),
          'contract_number' => array(
            '#type' => 'textfield',
            '#default_value' => isset($settings['contracts']['sellpoints'][$key][$type]['contract_number']) ? $settings['contracts']['sellpoints'][$key][$type]['contract_number'] : '',
            '#disabled' => TRUE,
            '#description' => t('Contract number for @label payment', array('@label' => $settings['contracts']['sellpoints'][$key][$type]['label'])),
          ),
          'primary' => array(
            '#type' => 'checkbox',
            '#title' => t('Primary'),
            '#default_value' => $settings['contracts']['sellpoints'][$key][$type]['primary'],
          ),
          'secondary' => array(
            '#type' => 'checkbox',
            '#title' => t('Secondary'),
            '#default_value' => $settings['contracts']['sellpoints'][$key][$type]['secondary'],
          ),
          'label' => array(
            '#type' => 'value',
            '#value' => $settings['contracts']['sellpoints'][$key][$type]['label'],
          ),
        );
      }
    }
  }

  // give access on the current setted sellpoint contracts list
  if (isset($settings['contracts']['sellpoint']) && isset($form['contracts']['sellpoints'][$settings['contracts']['sellpoint']])) {
    $sellPoint = $settings['contracts']['sellpoint'];
    foreach ($form['contracts']['sellpoints'][$sellPoint] as $type => $fields) {
      $form['contracts']['sellpoints'][$sellPoint][$type]['#access'] = TRUE;
      foreach ($form['contracts']['sellpoints'][$sellPoint][$type] as $key => $elt) {
        if (strpos($key, '#') === FALSE) {
          $form['contracts']['sellpoints'][$sellPoint][$type][$key]['#access'] = TRUE;
        }
      }
    }
  }

  $form['customisation'] = array(
    '#disabled' => TRUE,
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#title' => t('Customisations'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('You need to get your contracts before you can set these fields. Set your access and proxy (if required by your system) and click on validate button.'),
    'custom_description' => array(
      '#type' => 'textarea',
      '#title' => t('Description'),
      '#description' => t('You can set a description for this payment method. It will be displayed on checkout form'),
      '#default_value' => isset($settings['customisation']['custom_description']) ? $settings['customisation']['custom_description'] : '',
    ),
    'custom_template_url' => array(
      '#type' => 'textfield',
      '#title' => t('Template URL'),
      '#description' => t("Payment page's template url"),
      '#default_value' => isset($settings['customisation']['custom_template_url']) ? $settings['customisation']['custom_template_url'] : '',
    ),
    'custom_customisation_id' => array(
      '#type' => 'textfield',
      '#title' => t('Customisation Id'),
      '#default_value' => isset($settings['customisation']['custom_customisation_id']) ? $settings['customisation']['custom_customisation_id'] : '',
    ),
  );

  $form['payline_step_check'] = array(
    '#type' => 'button',
    '#tree' => TRUE,
    '#value' => t('Validate'),
    '#ajax' => array(
      'callback' => 'commerce_paylinebymonext_ui_form_reformat',
      'wrapper' => 'edit-parameter-payment-method',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );
  return $form;
}

/**
 * @param $form
 * @param $form_state
 * @return mixed
 */
function commerce_paylinebymonext_ui_form_reformat(&$form, &$form_state) {
  $method_id = $form_state['values']['parameter']['payment_method']['method_id'];
  $settings = $form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings'];
  if (!isset($form_state['paylinebymonext']['storage']['fullassist']) || empty($form_state['paylinebymonext']['storage']['fullassist'])) {
    module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.contracts');
    $form_state['paylinebymonext']['storage']['fullassist'] = commerce_paylinebymonext_contracts_remote_call($method_id, $settings, $form, $form_state);
  }
  else {
    if (isset($settings['contracts']['sellpoint'])) {
      $form_state['paylinebymonext']['storage']['sellpoint'] = $settings['contracts']['sellpoint'];
      $form_state['paylinebymmonext']['storage']['fullformloaded'] = TRUE;
    }
  }
  $form_state['rebuild_info']['copy']['#build_id'] = TRUE;
  $form['parameter']['payment_method']['#attributes']['id'] = 'edit-parameter-payment-method';

  return $form['parameter']['payment_method'];
}

/**
 * validate function for commerce_paylinebymonext_ui_settings_form
 *
 * @param $form
 * @param $form_state
 */
function _commerce_paylinebymonext_ui_settings_form_validate_process(&$form, &$form_state) {
  // check if validate function is processed during ajax process or not
  if ($form_state['triggering_element']['#type'] == 'submit') {
    $payment_method =& $form_state['values']['parameter']['payment_method']['settings']['payment_method'];
    module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.payments');
    module_load_include('inc', 'commerce_paylinebymonext', 'includes/commerce_paylinebymonext.contracts');
    $payment_method =& $form_state['values']['parameter']['payment_method']['settings']['payment_method'];
    $contracts = $payment_method['settings']['contracts'];
    $contracts_info = commerce_paylinebymonext_contracts_format($contracts);
    // check if at least one primary contracts is filled
    if (empty($contracts_info['primary_contracts'])) {
      commerce_paylinebymonext_ui_contracts_set_error($form, $form_state, $payment_method, t('You have to choose at least one primary contracts'));
    }
    else {
      // check if informations are validated on payline
      $payment_method['settings']['primary_contracts'] = $contracts_info['primary_contracts'];
      $payment_method['settings']['secondary_contracts'] = $contracts_info['secondary_contracts'];
      commerce_paylinebymonext_ui_account_informations_validate_process($form, $form_state, $payment_method);
    }
  }
}

/**
 * submit function for commerce_paylinebymonext_ui_settings_form
 *
 * @param array $form
 * @param array $form_state
 */
function _commerce_paylinebymonext_ui_settings_form_submit_process(&$form, &$form_state) {
  // check if validate function is processed during ajax process or not
  // check if validate function is processed during ajax process or not
  $payment_method =& $form_state['values']['parameter']['payment_method']['settings']['payment_method'];
  $primary_serial = serialize($payment_method['settings']['primary_contracts']);
  $secondary_serial = serialize($payment_method['settings']['secondary_contracts']);
  // store the reformated contract settings in the commerce_paylinebymonext_custom_settings table
  db_merge('commerce_paylinebymonext_custom_settings')
    ->key(array('method_id' => $payment_method['method_id']))
    ->fields(array(
      'primary_contracts' => $primary_serial,
      'secondary_contracts' => $secondary_serial,
    ))
    ->execute();
  $sellpoint = $form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings']['contracts']['sellpoint'];
  $args = array(
    '@primary' => count($payment_method['settings']['primary_contracts']),
    '@secondary' => count($payment_method['settings']['secondary_contracts']),
    '@sellpoint' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['contracts']['sellpoint']['#options'][$sellpoint],
  );
  drupal_set_message(t('Your ecommerce app is setted on Payline @sellpoint', $args));
  drupal_set_message(format_plural($args['@primary'], '@count primary contract is setted', '@count primary contracts are setted'));
  if ($args['@secondary'] > 0) {
    drupal_set_message(format_plural($args['@secondary'], '@count secondary contract is setted', '@count secondary contracts are setted'));
  }
  if (isset($payment_method['settings']['customisation']['custom_template_url']) && !empty($payment_method['settings']['customisation']['custom_template_url'])) {
    $args['@templateUrl'] = $payment_method['settings']['customisation']['custom_template_url'];
    drupal_set_message(t('custom template url : @templateUrl', $args));
  }
  if (isset($payment_method['settings']['customisation']['custom_customisation_id']) && !empty($payment_method['settings']['customisation']['custom_customisation_id'])) {
    $args['@customisationID'] = $payment_method['settings']['customisation']['custom_customisation_id'];
    drupal_set_message(t('Customisation ID : @customisationID', $args));
  }
  if ($payment_method['settings']['access']['production'] == 0) {
    drupal_set_message(t('Your drupal commerce site is plugged on payline test environnement'), 'warning');
  }
  variable_del('commerce_paylinebymonext_method_reset');
}

/**
 * validate information on payline account
 *
 * @param array $form
 * @param array $form_state
 * @param array $payment_method
 */
function commerce_paylinebymonext_ui_account_informations_validate_process(&$form, &$form_state, &$payment_method) {
  // Simulate an empty commerce order object
  $order = new stdClass();
  $order->order_id = 0;
  $order->commerce_order_total = array(
    LANGUAGE_NONE => array(
      array(
        'amount' => 100,
        'currency_code' => 'EUR',
      ),
    ),
  );
  // call a doWebPayment WS to check payment method settings on payline side
  $response = commerce_paylinebymonext_doWebPayment($payment_method, $order);
  commerce_paylinebymonext_ui_account_informations_validate($form, $form_state, $payment_method, $response);
}

/**
 * Validate payment method settings from a payline response
 *
 * @param array $form
 * @param array $form_state
 * @param array $payment_method
 * @param array $response
 *  The payline response
 */
function commerce_paylinebymonext_ui_account_informations_validate(&$form, &$form_state, $payment_method, $response) {
  $result = $response['result'];
  $element = NULL;
  $message = '';
  switch ($result['code']) {
    case COMMERCE_PAYLINE_CONNEXIONERROR:
      $element = '';
      $message = '';
      commerce_paylinebymonext_ui_connexion_error_get_details($element, $message, $result);
      break;
    case COMMERCE_PAYLINE_UNVALID_CONTRACT:
      $message = t('At least one of your contracts contract number is incorrect');
      commerce_paylinebymonext_ui_contracts_set_error($form, $form_state, $payment_method, $message);
      break;
    case COMMERCE_PAYLINE_UNFORMATEDFIELD:
      switch ($result['longMessage']) {
        case COMMERCE_PAYLINE_UNVALID_CUSTOMPAYEMENTPAGECODE:

          form_error($form['parameter']['payment_method']['settings']['payment_method']['settings']['customisation']['custom_customisation_id'], t('Your custom payment page code is not valid'));
          break;
        default :
          if (strpos($result['longMessage'], 'Invalid contract number') > 0) {
            $message = t('At least one of your contracts contract number is incorrect');
            commerce_paylinebymonext_ui_contracts_set_error($form, $form_state, $payment_method, $message);
          }
          elseif (strpos($result['longMessage'], 'Template URL') > 0) {
            form_error($form['parameter']['payment_method']['settings']['payment_method']['settings']['customisation']['custom_template_url'], t('Template URL : Must be an https:// url on port 443'));
          }
          break;
      }
      break;
  }
  if ($element) {
    commerce_paylinebymonext_ui_set_error($form, $form_state, $payment_method, $message, $element);
  }
}

/**
 * set error for contracts
 *
 * @param array $form
 * @param array $form_state
 * @param array $payment_method
 * @param string $message
 */
function commerce_paylinebymonext_ui_contracts_set_error(&$form, &$form_state, $payment_method, $message) {
  commerce_paylinebymonext_ui_set_error($form, $form_state, $payment_method, $message, 'contracts');
}

/**
 * form error message handler
 *
 * @param array $form
 * @param array $form_state
 * @param array $payment_method
 * @param string $message
 * @param string $element
 *  the form element has to be set in errorkey id
 * @param string $subfield
 *  the form subelement has to be set in error key id
 */
function commerce_paylinebymonext_ui_set_error(&$form, &$form_state, $payment_method, $message, $element, $subfield = NULL) {
  form_set_error($element, check_plain($message));
  if (isset($form['parameter']['payment_method']['settings']['payment_method']['settings'][$element]) && $form['parameter']['payment_method']['settings']['payment_method']['settings'][$element]['#type'] == 'fieldset') {
    $form['parameter']['payment_method']['settings']['payment_method']['settings'][$element]['#attributes']['class'][] = 'error';
    foreach ($payment_method['settings'][$element] as $subelement => $infos) {
      if ($subfield) {
        form_error($form['parameter']['payment_method']['settings']['payment_method']['settings'][$element][$subelement][$subfield]);
      }
      else {
        form_error($form['parameter']['payment_method']['settings']['payment_method']['settings'][$element][$subelement]);
      }
    }
  }
}

/**
 * Get details for a connexion error with payline service and set concerning element and appropriate message
 *
 * @param string $element
 *  will be override by this function
 * @param string $message
 *  will be override by this function
 * @param array $result
 *  the result part of a payline response
 */
function commerce_paylinebymonext_ui_connexion_error_get_details(&$element, &$message, $result) {
  if ($result['longMessage'] == COMMERCE_PAYLINE_NOCONNEXION) {
    $element = 'proxy';
    $message = t(COMMERCE_PAYLINE_NOCONNEXION . ' : Your proxy settings seems incorrect.');
  }
  else {
    $element = 'access';
    $message = t(COMMERCE_PAYLINE_UNAUTHORIZED . ' : Your access settings are incorrect.');
  }
}
