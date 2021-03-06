<?php

/**
 * @file
 * Provides a file attachment search implementation for use with the Apache Solr module
 */

/**
 * Menu callback: Apache Solr Attachments settings tab.
 */
function apachesolr_attachments_admin_page($environment = NULL) {
  if (empty($environment)) {
    $env_id = apachesolr_default_environment();
    $environment = apachesolr_environment_load($env_id);
  }
  else {
    $env_id = $environment['env_id'];
  }
  $output['apachesolr_attachments_settings'] = drupal_get_form('apachesolr_attachments_settings', $env_id);
  $output['apachesolr_attachments_index_action_form'] = drupal_get_form('apachesolr_attachments_index_action_form', $env_id);
  return $output;
}

/**
 * Displays the Attachment Settings Form.
 *
 * @see apachesolr_attachments_settings_validate()
 * @see apachesolr_attachments_settings_submit()
 */
function apachesolr_attachments_settings($form, &$form_state, $env_id) {
  $default = implode(' ', apachesolr_attachments_default_excluded());
  $form['apachesolr_attachments_excluded_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Excluded file extensions'),
    '#default_value' => variable_get('apachesolr_attachments_excluded_extensions', $default),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('File extensions that are excluded from indexing. Separate extensions with a space and do not include the leading dot. Extensions are internally mapped to a MIME type, so it is not necessary to put variations that map to the same type (e.g. tif is sufficient for tif and tiff)'),
  );
  $form['apachesolr_attachments_extract_using'] = array(
    '#type' => 'radios',
    '#title' => t('Extract using'),
    '#options' => array(
      'tika' => t('Tika (local java application)'),
      'solr' => t('Solr (remote server)'),
    ),
    '#description' => t("Extraction will be faster if run locally using tika."),
    '#default_value' => variable_get('apachesolr_attachments_extract_using', 'tika'),
  );
  $form['apachesolr_attachments_filesize_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('File size limit (in bytes) for files to be indexed'),
    '#size' => 10,
    '#description' => t('If a file is larger than this limit, do not index it. Default is 41943040 bytes (40MB).'),
    '#default_value' => variable_get('apachesolr_attachments_filesize_limit', '41943040'),
  );
  $form['apachesolr_attachments_tika_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Tika directory path'),
    '#size' => 80,
    '#maxlength' => 100,
    '#description' => t("The full path to the tika directory. All library jars must be in the same directory. If on Windows, use forward slashes in the path."),
    '#default_value' => variable_get('apachesolr_attachments_tika_path', ''),
  );
  $form['apachesolr_attachments_tika_jar'] = array(
    '#type' => 'textfield',
    '#title' => t('Tika jar file'),
    '#size' => 20,
    '#description' => t("The name of the tika CLI application jar file, e.g. tika-app-1.1.jar."),
    '#default_value' => variable_get('apachesolr_attachments_tika_jar', 'tika-app-1.1.jar'),
  );

  $form = system_settings_form($form);
  $form['#validate'][] = 'apachesolr_attachments_settings_validate';
  $form['#submit'][] = 'apachesolr_attachments_settings_submit';
  return $form;
}

/**
 * Form validation for the Apache Solr Attachments settings form.
 *
 * @see apachesolr_attachments_settings()
 */
function apachesolr_attachments_settings_validate($form, &$form_state) {
  if ($form_state['values']['apachesolr_attachments_extract_using'] == 'tika') {
    $path = realpath($form_state['values']['apachesolr_attachments_tika_path']);
    if (!file_exists($path . '/' . $form_state['values']['apachesolr_attachments_tika_jar'])) {
      form_set_error('apachesolr_attachments_tika_path', t('Tika jar file not found at this path.'));
    }
  }
  if (!is_numeric($form_state['values']['apachesolr_attachments_filesize_limit'])) {
    form_set_error('apachesolr_attachments_filesize_limit', t('File size limit must be a number (in bytes).'));
  }
}

/**
 * Form submit handler for the settings Form
 *
 * @see apachesolr_attachments_settings()
 */
function apachesolr_attachments_settings_submit($form, &$form_state) {
  // Delete this so it's rebuilt.
  variable_del('apachesolr_attachments_excluded_mime');
  drupal_set_message(t('If you changed the allowed file extensions, you may need to delete and re-index all attachments.'));
}

/**
 * Form callback for content type settings.
*/
function apachesolr_attachments_entity_bundle_settings() {
  $env_id = apachesolr_default_environment();

  foreach (entity_get_info() as $entity_type => $entity_info) {
    if (!empty($entity_info['apachesolr']['indexable'])) {
      $indexed_bundles = apachesolr_get_index_bundles($env_id, $entity_type);

      foreach ($indexed_bundles as $bundle) {
        if ($bundle != 'file') {
          $form['apachesolr_attachments_entity_bundle_indexing_' . $bundle] = array(
            '#type' => 'select',
            /* NOTE: If http://drupal.org/node/969180 ever gets committed we could use that function instead
            and remove apachesolr_attachments_entity_bundle_label from the bottom of this file.
            */
            '#title' => apachesolr_attachments_entity_bundle_label($entity_type, $bundle),
            '#default_value' => variable_get('apachesolr_attachments_entity_bundle_indexing_' . $bundle, 'seperate'),
            '#options' => array(
              'seperate' => t('Attachments as separate entities'),
              'parent' => t('Attachments as part of parent entity'),
              'none' => t("Don't index attachments"),
            ),
          );
        }
      }
    }
  }

  if (empty($form)) {
    // Display a message if there are no indexable bundles.
    $form['no_bundles']['#markup'] = t('There are no bundles indexable by Apache Solr. You can select entities and bundles that should be indexed via <a href="!url">the Apache Solr search settings</a> page.', array('!url' => url('admin/config/search/apachesolr')));
    return $form;
  }
  else {
    return system_settings_form($form);
  }
}

/**
 * Form builder for the Apachesolr Attachments actions form.
 *
 */
function apachesolr_attachments_index_action_form($form, &$form_state, $env_id) {
  $form = array();
  $form['action'] = array(
    '#type' => 'fieldset',
    '#title' => t('Actions'),
    '#collapsible' => TRUE,
  );

  $form['action']['env_id'] = array(
    '#type' => 'value',
    '#value' => $env_id,
  );

  $form['action']['reset'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#value' => t('Clear the attachment text extraction cache'),
    '#submit' => array('apachesolr_attachments_index_action_form_reset_submit'),
  );
  $form['action']['delete'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#value' => t('Delete the attachments from the index'),
    '#submit' => array('apachesolr_attachments_index_action_form_delete_submit'),
  );
  $form['action']['extract'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#value' => t('Test your tika extraction'),
    '#submit' => array('apachesolr_attachments_index_action_form_extraction_submit'),
  );
  return $form;
}

/**
 * Submit handler for the Indexer actions form, test button.
 */
function apachesolr_attachments_index_action_form_extraction_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $env_id = $form_state['values']['env_id'];
  $form_state['redirect'] = array('admin/config/search/apachesolr/attachments/test', array('query' => $destination));
}

/**
 * Submit handler for the Indexer actions form, reset button.
 */
function apachesolr_attachments_index_action_form_reset_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $env_id = $form_state['values']['env_id'];
  $form_state['redirect'] = array('admin/config/search/apachesolr/attachments/confirm/clear-cache', array('query' => $destination));
}

/**
 * Submit handler for the Indexer actions form, delete button.
 */
function apachesolr_attachments_index_action_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $env_id = $form_state['values']['env_id'];
  $form_state['redirect'] = array('admin/config/search/apachesolr/attachments/confirm/delete', array('query' => $destination));
}

/**
 * Index confirmation form
 *
 * @see apachesolr_attachments_confirm_submit()
 */
function apachesolr_attachments_confirm($form, $form_state, $operation) {
  $form = array();
  $form['operation'] = array(
    '#type' => 'value',
    '#value' => $operation,
  );
  switch ($operation) {
    case 'delete':
      $text = t('Are you sure you want to delete and re-index the text of all file attachments?');
      break;
    case 'clear-cache':
      $text = t('Are you sure you want to delete the cache of extracted text from file attachments?');
      break;
  }
  return confirm_form($form, $text, 'admin/config/search/apachesolr/attachments', NULL, t('Confirm'), t('Cancel'));
}

/**
 * Form submit handler for the index confirmation form
 *
 * @see apachesolr_attachments_confirm()
 */
function apachesolr_attachments_confirm_submit($form, &$form_state) {
  switch ($form_state['values']['operation']) {
    case 'delete':
      if (apachesolr_attachments_delete_index() && apachesolr_attachments_solr_reindex()) {
        drupal_set_message(t('File text has been deleted from the Apache Solr index. You must now <a href="@url">run cron</a> until all files have been re-indexed.', array('@url' => url('admin/reports/status/run-cron', array('query' => array('destination' => 'admin/config/search/apachesolr/index'))))));
      }
      else {
        if (module_exists('dblog')) {
          drupal_set_message(t('Could not delete file text from the Apache Solr index. Check <a href="@url">recent log messages</a>.', array('@url' => url('admin/reports/dblog'))));
        }
        else {
          drupal_set_message(t('Could not delete file text from the Apache Solr index.'));
        }
      }
      break;
    case 'clear-cache':
      apachesolr_attachments_solr_reindex();
      drupal_set_message(t('The local cache of extracted text has been deleted.'));
      break;
  }
  $form_state['redirect'] = 'admin/config/search/apachesolr/attachments';
}

/**
 * Function to test if our extracting with tika succeeds
 */
function apachesolr_attachments_test_tika_extraction() {
  module_load_include('inc', 'apachesolr_attachments', 'apachesolr_attachments.index');
  $indexer_table = apachesolr_get_indexer_table('file');
  // Create new file
  $file = new stdClass();
  $file->uri = drupal_get_path('module', 'apachesolr_attachments') . '/tests/test-tika.pdf';
  $file->filemime = 'application/pdf';
  $file->fid = 0;
  $text = apachesolr_attachments_get_attachment_text($file);
  // Check if the text can be succesfully extracted. Only checking 1 word is
  // sufficient
  if (strpos($text, 'extraction')) {
    drupal_set_message(t('Text can be succesfully extracted'));
  }
  else {
    drupal_set_message(t('Text can not be succesfully extracted. Please check your settings'), 'error');
  }
  // Delete our test file from indexing table
  db_delete($indexer_table)->condition('entity_id', $file->fid)->execute();

  drupal_goto('admin/config/search/apachesolr/attachments');
}

/**
 * @see apachesolr_delete_index()
 */
function apachesolr_attachments_delete_index() {
  try {
    $solr = apachesolr_get_solr();
    $solr->deleteByQuery("entity_type:file AND hash:" . apachesolr_site_hash());
    module_load_include('inc', 'apachesolr', 'apachesolr.index');
    apachesolr_attachments_solr_reindex();
    return TRUE;
  }
  catch (Exception $e) {
    watchdog('Apache Solr Attachments', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
  }
  return FALSE;
}


/**
 * NOTE: IF http://drupal.org/node/969180 ever gets committed we could do away with this function
 *
 * Returns the label of a bundle.
 *
 * @param $entity_type
 *   The entity type; e.g. 'node' or 'user'.
 * @param $entity
 *   The entity for which we want the human-readable label of its bundle.
 *
 * @return
 *   A string with the human-readable name of the bundle, or FALSE if not specified.
 */
function apachesolr_attachments_entity_bundle_label($entity_type, $bundle) {
  $labels = &drupal_static(__FUNCTION__, array());

  if (empty($labels)) {
    foreach (entity_get_info() as $entity_t => $entity_info) {
      foreach ($entity_info['bundles'] as $b => $bundle_info) {
        $labels[$entity_t][$b] = !empty($bundle_info['label']) ? $bundle_info['label'] : $b;
      }
    }
  }

  return isset($labels[$entity_type][$bundle]) ? $labels[$entity_type][$bundle] : $bundle;
}
