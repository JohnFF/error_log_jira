<?php

/**
 * @file
 * Functions file for Error Log Jira module.
 *
 * A module that takes the log messages from watchdog and send them as
 * issues to jira.
 */

/**
 * Select the items, from watchdog leftjoined with error_log_jira.
 *
 * @param string $number
 *   The number that the user has selected. This number shows how many values
 *   will be checked from watchdog on every cron run.
 *
 * @param array $severity_levels
 *   An array with the severities, and their ids, that the user has selected
 *   in checkboxes.
 *
 * @return array
 *   $result Items from watchdog.
 */
function error_log_jira_items_from_watchdog($number, array $severity_levels) {

  $query = db_select('watchdog', 'wa')
    ->condition('wa.severity', array_keys($severity_levels), 'IN');
  $query->leftJoin('error_log_jira', 'j', 'j.wid = wa.wid');
  $query->isNull('j.resolution')
    ->fields('wa')
    ->fields('j', array('resolution', 'resolution_updated'))
    ->range(0, $number)
    ->orderBy('j.resolution_updated', 'ASC');
  $result = $query->execute()->fetchall();

  return $result;
}

/**
 * Creates the table with the data that will be send to jira.
 *
 * @param object $watchdog_item
 *   Item from watchdog with all information that is needed to create the jira
 *   issue.
 * @param string $severity_name
 *   The name of the watchdog severity for the existing watchdog item that is
 *   passed.
 * @param array $options
 *   An array with the username and the password from jira_rest module.
 * @param string $project_key
 *   The key of the jira project.
 */
function error_log_jira_update_db_and_jira($watchdog_item, $severity_name, array $options, $project_key) {

  $timestamp = $watchdog_item->timestamp;
  $wid       = $watchdog_item->wid;
  // Checks what message needed.
  if (isset($watchdog_item->message) && isset($watchdog_item->variables)) {
    if ($watchdog_item->variables === 'N;') {
      $db_message = $watchdog_item->message;
    }
    else {
      $db_message = format_string($watchdog_item->message, unserialize($watchdog_item->variables));
      $db_message = filter_xss($db_message, array());
    }
  }

  // Check if a value exist in the table.
  $select_same_variables = db_select('error_log_jira', 'j')
    ->fields('j')
    ->condition('wid', $wid, '=')
    ->execute();

  // If the value does not exists in the table, then creates this value.
  if ($select_same_variables->rowCount() == 0) {
    db_insert('error_log_jira')
      ->fields(array(
        'timestamp'          => $timestamp,
        'wid'                => $wid,
        'message'            => $db_message,
        'resolution_updated' => time(),
        'resolution'         => 'Unresolved',
      ))
      ->execute();

    // Looks for the item of the watchdog if is Resolved or not.
    $jql_string  = ('project = ' . $project_key .
      ' and description ~ "\"' . $db_message . '\"" and resolution = Unresolved');
    $jira_search = jira_rest_issue_search($jql_string, $options);

    // If this item of watchdog is Unresolved, then do nothing.
    if (empty($jira_search)) {
      error_log_jira_create_issue($watchdog_item, $severity_name, $options, $project_key);
    }
  }
}

/**
 * Issuedata that will create the issue in jira.
 *
 * @param object $watchdog_item
 *   Item from watchdog with all information that is needed to create the jira
 *   issue.
 * @param string $level
 *   The name of the watchdog severity for the existing watchdog item that is
 *   passed.
 * @param array $options
 *   An array with the username and the password from jira_rest module.
 * @param string $project_key
 *   The key of the jira project.
 */
function error_log_jira_create_issue($watchdog_item, $level, array $options, $project_key) {

  $type = $watchdog_item->type;
  $wid  = $watchdog_item->wid;
  // Checks what message is needed.
  if (isset($watchdog_item->message) && isset($watchdog_item->variables)) {
    if ($watchdog_item->variables === 'N;') {
      $db_message = $watchdog_item->message;
    }
    else {
      $db_message = format_string($watchdog_item->message, unserialize($watchdog_item->variables));
      $db_message = filter_xss($db_message, array());
    }
  }

  // Issuedata that will be send to jira project as issue.
  $issue_data = array(
    'fields' => array(
      'project'     => array('key' => $project_key),
      'summary'     => '[' . $wid . '] ' . $type . ' - ' . $level,
      'description' => $db_message,
      'priority'    => array('id' => '3'),
      'issuetype'   => array('id' => '1'),
    ),
  );
  $jql_string  = ('project = ' . $project_key . ' and summary ~ ' . $wid);
  $jira_search = jira_rest_issue_search($jql_string, $options);

  // If this item of watchdog is Unresolved, then do nothing.
  if (empty($jira_search)) {
    // Calls a function, that creates the issue in jira, from the jira_rest
    // module.
    jira_rest_issue_create($issue_data, $options);
  }
}

/**
 * Get the resolution from jira and update the columns of resolution and
 * resolution_updated in error_log_jira table.
 *
 * @param array $options
 *   An array with the username and the password from jira_rest module.
 * @param string $project_key
 *   The key of the jira project.
 */
function error_log_jira_update_resolution(array $options, $project_key) {

  // Update resolution and resolution_updated columns from error_log_jira table.
  $jql_string  = ('project = ' . $project_key . ' and resolution != Unresolved');
  $jira_issues = jira_rest_issue_search($jql_string, $options);
  foreach ($jira_issues as $jira_issue) {
    if (($jira_issue->fields->resolution)) {
      db_update('error_log_jira')
        ->condition('message', $jira_issue->fields->description, '=')
        ->fields(array(
          'resolution'         => $jira_issue->fields->resolution->name,
          'resolution_updated' => time(),
        ))
        ->execute();
    }
  }
}

/**
 * Keep, in error_log_jira table, equal amount of items as in watchdog table.
 */
function error_log_jira_keep_latest_records() {
  // This number is from /admin/config/development/logging and shows how many
  // items keep the watchdog table.
  $number = variable_get('dblog_row_limit');

  // Select, in error_log_jira table, equal number of items as in watchdog.
  $query = db_select('error_log_jira', 'j')
    ->fields('j')
    ->range(0, $number)
    ->orderBy('j.resolution_updated', 'ASC');
  $ids = $query->execute()->fetchCol('wid');

  // Delete all the other values.
  db_delete('error_log_jira')
    ->condition('wid', $ids, 'NOT IN')
    ->execute();
}
