<?php

/**
 * @file
 * Override paylineSDK class
 */

module_load_include('php', 'commerce_paylinebymonext', 'library/paylineSDK');

/**
 * Class commercePaylineSDK
 */
class commercePaylineSDK extends paylineSDK {
  protected $_watchlog = NULL;

  /**
   * Override parent method writeTrace()
   *  connect paylineSDK log system on drupal watchdog.
   *
   * @param $trace
   */
  public function writeTrace($trace) {
    if ($this->_watchlog === NULL) {
      $this->_watchlog = array('start payline call');
    }
    $this->_watchlog[] = $trace;
  }

  /**
   * log the temp log entries in drupal watchdog
   */
  public function saveLog() {
    if ($this->_watchlog !== NULL) {
      // set correct glue to format log message
      $glue = ' # ';
      if (module_exists('dblog')) {
        $glue = '<br>';
      }
      elseif (module_exists('syslog')) {
        $glue = "\n";
      }
      // format log message
      $message = implode($glue, $this->_watchlog);
      // check severity
      $severity = WATCHDOG_NOTICE;
      if (strpos($message, 'Exception :')) {
        $severity = WATCHDOG_ERROR;
      }
      // log and flush temp log entry
      watchdog(COMMERCE_PAYLINE_WD, $message, array(), $severity);
      $this->flushLog();
    }
  }

  /**
   * flush temp log entries
   */
  protected function flushLog() {
    $this->_watchlog = NULL;
  }
}