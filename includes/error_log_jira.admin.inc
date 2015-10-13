<?php

/**
 * @file
 * Admin settings file for Error Log Jira module.
 *
 * A module that takes the log messages from watchdog and send them as
 * issues to jira.
 */

/**
 * Admin settings form.
 */
function error_log_jira_settings_form($form, &$form_state) {
  // Url from jira_rest module.
  $url = variable_get('jira_rest_jirainstanceurl', 'https://localhost:8443');

  // Check if the url field of jira_rest module has the default value.
  if ($url == 'https://yourjira.com:8443') {
    drupal_set_message(t("You have to complete the fields in !link first."),
      array('!link' => l(t("jira_rest"), 'admin/config/services/jira_rest')), 'error');
  }
  else {
    // Form for the key for the jira project.
    $form['error_log_jira_key'] = array(
      '#type'          => 'textfield',
      '#title'         => t('jira Project KEY'),
      '#default_value' => variable_get('error_log_jira_key', ''),
      '#description'   => t('Enter the KEY of your jira project.<br>
                           (NOT THE TITLE).'),
      '#required'      => TRUE,
      '#size'          => 10,
    );

    // The select form.
    $error_log_jira_options = array(
      '0'  => 'No Values',
      '1'  => '1',
      '6'  => '6',
      '10' => '10',
      '25' => '25',
    );
    $form['error_log_jira_number_select'] = array(
      '#type'          => 'select',
      '#title'         => t('Number of items to be checked.'),
      '#options'       => $error_log_jira_options,
      '#description'   => t('Select how many values do you want<br> to be
                             checked on every cron run.'),
      '#default_value' => variable_get('error_log_jira_number_select', '0'),
    );

    // Keep the values that the user selected.
    $error_log_jira_severity_levels = variable_get('error_log_jira_severity_levels', array());

    // Declares empty options array.
    $error_log_jira_severity_options = array();

    // Loop through each watchdog_severity_levels and create a severity_options
    // array().
    foreach (watchdog_severity_levels() as $number_select => $severity) {
      $error_log_jira_severity_options[$severity] = drupal_ucfirst($severity);
      $number_select == $number_select;
    }

    // Checkboxes form.
    $form['error_log_jira_severity_levels'] = array(
      '#type'          => 'checkboxes',
      '#title'         => t('Type of log messages'),
      '#options'       => $error_log_jira_severity_options,
      '#default_value' => array_values($error_log_jira_severity_levels),
      '#required'      => TRUE,
    );

    // Submit form for the "Save" button.
    $form['submit'] = array(
      '#type'  => 'submit',
      '#value' => t('Save'),
    );

    return $form;
  }
}

/**
 * Submit handler settings form.
 */
function error_log_jira_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];

  // Join the values from checkboxes with watchdog_severity_levels().
  $result = array_intersect(watchdog_severity_levels(), $values['error_log_jira_severity_levels']);

  // Save the values.
  variable_set('error_log_jira_key', $values['error_log_jira_key']);
  variable_set('error_log_jira_number_select', $values['error_log_jira_number_select']);
  variable_set('error_log_jira_severity_levels', $result);

  // Show a message of success.
  drupal_set_message(t('Settings saved successfully.'));
}
