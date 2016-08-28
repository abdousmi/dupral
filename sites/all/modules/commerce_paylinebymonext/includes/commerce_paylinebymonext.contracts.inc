<?php

/**
 * @file
 *  Provide api function to handle contracts features
 */

/**
 * Return remote info for contracts from payline servers
 *
 * @param string $method_id
 * @param array $settings
 * @return array
 */
function commerce_paylinebymonext_contracts_get_remote_info($method_id, $settings = array()) {
  if (empty($settings)) {
    // load the full used payment method instance
    $payment_method = commerce_paylinebymonext_payment_method_load($method_id);
    $settings = $payment_method['settings'];
  }
  // Set the commercePaylineSDK object to handle the WS call
  $payline = commerce_paylinebymonext_sdk($settings);
  $response = $payline->getMerchantSettingsToArray(null);
  $payline->saveLog();
  return $response;
}

/**
 * @param $form
 * @return array
 */
function commerce_paylinebymonext_contracts_prepare_params($form) {
  $params = array(
    'access' => array(
      'merchant_id' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['access']['merchant_id']['#default_value'],
      'access_key' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['access']['access_key']['#default_value'],
      'production' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['access']['production']['#default_value'],
    ),
    'proxy' => array(
      'proxy_host' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['proxy']['proxy_host']['#default_value'],
      'proxy_port' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['proxy']['proxy_port']['#default_value'],
      'proxy_user' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['proxy']['proxy_user']['#default_value'],
      'proxy_pwd' => $form['parameter']['payment_method']['settings']['payment_method']['settings']['proxy']['proxy_pwd']['#default_value'],
    ),
    'method_id' => $form['parameter']['payment_method']['settings']['payment_method']['method_id']['#value'],
  );
  return $params;
}

/**
 * @param $method_id
 * @param $settings
 * @param array $form
 * @param array $form_state
 * @return null
 */
function commerce_paylinebymonext_contracts_remote_call($method_id, $settings, $form = array(), $form_state = array()) {
  $remote_info = NULL;
  $response = commerce_paylinebymonext_contracts_get_remote_info($method_id, $settings);
  if ($response['result']['code'] == COMMERCE_PAYLINE_CONNEXIONERROR) {
    $element = '';
    $message= '';
    commerce_paylinebymonext_ui_connexion_error_get_details($element, $message, $response['result']);
    if (!empty($form)) {
      commerce_paylinebymonext_ui_set_error($form, $form_state, $form['parameter']['payment_method']['settings']['payment_method']['settings'], $message, $element);
    } else {
      drupal_set_message($message, 'warning');
    }
  } elseif ($response['result']['code'] == COMMERCE_PAYLINE_NOERROR) {
    $remote_info = $response['POS'];
  } else {
    if (!empty($form)) {
      form_error($form['parameter']['payment_method']['settings']['payment_method']['settings'], t('There is a connexion problem with Payline servers, try again later'));
    } else {
      $message= t('There is a connexion problem with payline servers, try to set your plateform later');
      drupal_set_message($message, 'warning');
    }
  }
  return $remote_info;
}

/**
 * Reformat contract type if it was rewritten on save contacts settings
 *
 * @see commerce_paylinebymonext_ui_contracts_fieldset_format()
 * @param string $type
 * @return string
 */
function commerce_paylinebymonext_contracts_reformat_type($type) {
  $logo_type = $type;
  $pos = strpos($type, '#');
  if ($pos !== false) {
    $logo_type = substr($type, 0, $pos);
  }
  return $logo_type;
}

/**
 * Get the complete list of available payment contracts on Payline
 *  with default name and logo
 *
 * @return array
 */
function commerce_paylinebymonext_contracts_types() {
  $path = drupal_get_path('module', 'commerce_paylinebymonext') . '/images/';

  $contracts = array(
    'mcvisa' => array(
      'name' => 'VISA',
      'path' => $path . 'VISA.gif',
    ),
    'amex' => array(
      'name' => 'American Express',
      'path' => $path . 'AMEX.gif',
    ),
    'sofinco' => array(
      'name' => 'Sofinco',
      'path' => $path . 'SOFINCO.gif',
    ),
    'diners' => array(
      'name' => 'Diners Club',
      'path' => $path . 'DINERS.gif',
    ),
    'aurore' => array(
      'name' => 'Aurore',
      'path' => $path . 'AURORE.gif',
    ),
    'pass' => array(
      'name' => 'VISA PASS',
      'path' => $path . 'PASS.gif',
    ),
    'cbpass' => array(
      'name' => 'PASS',
      'path' => $path . 'CBPASS.gif',
    ),
    'cofinoga' => array(
      'name' => 'Cofinoga',
      'path' => $path . 'COFINOGA.gif',
    ),
    'cdgp' => array(
      'name' => 'Carte Privilège',
      'path' => $path . 'CDGP.gif',
    ),
    'printemps' => array(
      'name' => 'Printemps',
      'path' => $path . 'PRINTEMPS.gif',
    ),
    'kangourou' => array(
      'name' => 'Kangourou',
      'path' => $path . 'KANGOUROU.gif',
    ),
    'surcouf' => array(
      'name' => 'Surcouf',
      'path' => $path . 'SURCOUF.gif',
    ),
    'cyrillus' => array(
      'name' => 'Cyrillus',
      'path' => $path . 'CYRILLUS.gif',
    ),
    'fnac' => array(
      'name' => 'Carte FNAC',
      'path' => $path . 'FNAC.gif',
    ),
    'jcb' => array(
      'name' => 'JCB',
      'path' => $path . 'JCB.gif',
    ),
    'maestro' => array(
      'name' => 'MAESTRO',
      'path' => $path . 'MAESTRO.gif',
    ),
    'switch' => array(
      'name' => 'switch',
      'path' => $path . 'SWITCH.gif',
    ),
    'uneuro' => array(
      'name' => '1EURO.COM',
      'path' => $path . '1EURO.COM.gif',
    ),
    'paypal' => array(
      'name' => 'Paypal',
      'path' => $path . 'PAYPAL.gif',
    ),
    'paysafecard' => array(
      'name' => 'Pay Safe Card',
      'path' => $path . 'PAYSAFECARD.gif',
    ),
    'casino' => array(
      'name' => 'Casino',
      'path' => $path . 'CASINO.gif',
    ),
    'skrill(moneybookers)' => array(
      'name' => 'Skrill',
      'path' => $path . 'SKRILL.gif',
    ),
    'mandarine' => array(
      'name' => 'Mandarine',
      'path' => $path . 'MANDARINE.gif',
    ),
    'okshopping' => array(
      'name' => 'OK Shopping',
      'path' => $path . 'OKSHOPPING.gif',
    ),
    'ticketsurf' => array(
      'name' => 'Ticket Surf',
      'path' => $path . 'TICKETSURF.gif',
    ),
    'elv' => array(
      'name' => 'ELV',
      'path' => $path . 'ELV.gif',
    ),
    'payfair' => array(
      'name' => 'Pay Fair',
      'path' => $path . 'PAYFAIR.gif',
    ),
    '3xcb' => array(
      'name' => '3x CB',
      'path' => $path . '3XCB.gif',
    ),
    'ideal' => array(
      'name' => 'iDeal',
      'path' => $path . 'IDEAL.gif',
    ),
    'internet+' => array(
      'name' => 'INTERNET+',
      'path' => $path . 'INTERNET+.gif',
    ),
    'buyster' => array(
      'name' => 'Buyster',
      'path' => $path . 'BUYSTER.gif',
    ),
    'sddsepa' => array(
      'name' => t('Banking Order'),
      'path' => $path . 'SDDSEPA.gif',
    ),
    'leetchi' => array(
      'name' => 'Leetchi',
      'path' => $path . 'LEETCHI.gif',
    ),
    'wexpay' => array(
      'name' => t('cash option'),
      'path' => $path . 'WEXPAY.gif',
    ),
    'maxicheque' => array(
      'name' => 'MAXICHEQUE',
      'path' => $path . 'MAXICHEQUE.gif',
    ),
    'moneo' => array(
      'name' => 'Moneo',
      'path' => $path . 'MONEO.gif',
    ),
    'neosurf' => array(
      'name' => 'NEOSURF',
      'path' => $path . 'NEOSURF.gif',
    ),
    'moneyclic' => array(
      'name' => 'MoneyClick',
      'path' => $path . 'MONEYCLIC.gif',
    ),
    'totalgr' => array(
      'name' => 'TOTAL GR',
      'path' => $path . 'TOTALGR.gif',
    ),
    'ukash' => array(
      'name' => 'UKASH',
      'path' => $path . 'UKASH.gif',
    ),
    'cb' => array(
      'name' => 'VISA MasterCard CB',
      'path' => $path . 'CB-VISA-MC.gif',
    ),
  );
  drupal_alter('commerce_paylinebymonext_contracts_types', $contracts);
  return $contracts;
}

/**
 * Return a rendarable array for logo contract
 *
 * @param $contract
 * @return array
 */
function commerce_paylinebymonext_contracts_prepare_picture($contract) {
  $img = array(
    '#theme' => 'image',
    '#path' => $contract['path'],
    '#alt' => t('@contract logo', array('@contract' => $contract['name'])),
  );
  return $img;
}

/**
 * Return full contract information for a enabled contract
 *  by contract number for a payment method
 *
 * @param array $payment_method
 * @param string $number
 * @return array
 */
function commerce_paylinebymonext_contracts_get_by_number($payment_method, $number) {
  $contract_types = commerce_paylinebymonext_contracts_types();
  $contract = array();
  // get the primary and secondary contracts if they are not in payment methods settings
  // this is possible as the drupal commerce out of box settings handler can't transform
  // informations from elements rules from to complex information as needed by Payline process.
  if (!isset($payment_method['settings']['primary_contracts'])) {
    $payment_method['settings']['method_id'] = $payment_method['method_id'];
    drupal_alter('commerce_paylinebymonext_settings', $payment_method['settings']);
  }
  foreach (array(
             'primary' => $payment_method['settings']['primary_contracts'],
             'secondary' => $payment_method['settings']['secondary_contracts']
           ) as $type => $contracts) {
    if (empty($contract)) {
      foreach ($contracts as $contract_code => $info) {
        if ($info['contract_number'] == $number) {
          $contract_code = commerce_paylinebymonext_contracts_reformat_type($contract_code);
          $contract = array(
            'name' => $contract_types[$contract_code]['name'],
            'contract_number' => $number,
            'type' => $type,
          );
        }
      }
    }
  }
  return $contract;
}

/**
 * Reformat contract settings
 *  As the drupal commerce out of box settings handler can't transform
 *  informations from elements rules form to complex information as needed by Payline process
 *  we have to reformat it to prepare insert in commerce_paylinebymonext_custom_settings table.
 *
 * @param $contracts
 * @return array
 */
function commerce_paylinebymonext_contracts_format($contracts) {
  $contract_info = array(
    'primary_contracts' => array(),
    'secondary_contracts' => array(),
  );
  $sellpoint = $contracts['sellpoint'];
  foreach ($contracts['sellpoints'][$sellpoint] as $contract => $infos) {
    if ($infos['primary'] == 1) {
      $contract_info['primary_contracts'][$contract] = $infos;
    }
    if ($infos['secondary'] == 1) {
      $contract_info['secondary_contracts'][$contract] = $infos;
    }
  }
  return $contract_info;
}

/**
 * Prepare contracts number array to be passed into WS param array
 *
 * @param array $settings
 *  the payment method settings array
 * @return array
 *  An array with primary and secondary contract number split into to subarray
 */
function commerce_paylinebymonext_contracts_number($settings) {
  $contracts_number = array(
    'primary_contracts' => array(),
    'secondary_contracts' => array(),
  );
  foreach ($contracts_number as $level => $ret) {
    if (!is_array($settings[$level])) {
      $settings[$level] = array();
    }
    foreach ($settings[$level] as $infos) {
      $contracts_number[$level][] = $infos['contract_number'];
    }
  }
  return $contracts_number;
}
