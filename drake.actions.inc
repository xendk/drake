<?php

/**
 * @file
 * Standard actions for Drake.
 */

function drake_standard_actions() {
  return array(
    'shell' => array(
      'callback' => 'drake_action_shell',
      'multiple' => 'command',
      'parameters' => array(
        'command' => 'Shell command to run.',
      ),
    ),
    'drush' => array(
      'callback' => 'drake_action_drush',
      'multiple' => 'command',
      'parameters' => array(
        'command' => array(
          'description' => 'The drush command to run.',
        ),
        'args' => array(
          'description' => 'Arguments to Drush command. Keyed values are switches, numerically keyed values are arguments.',
          'default' => array(),
        ),
        'target' => array(
          'description' => 'Site-alias to run the command on (default: @self)',
          'default' => '@self',
        ),
      ),
      'examples' => array(
        'sql-sync' => array(
          'action' => 'drush',
          'command' => 'sql-sync',
          'args' => array('@prod', '@dev', 'no-cache' => TRUE),
        ),
      ),
    ),
  );
}

function drake_action_error($message) {
  $context = &drush_get_context('DRAKE_RUN');
  $message = dt('Action "@action" failed: ', array('@action' => $context['action']['name'])) . $message;
  return drush_set_error('DRAKE_TASK_ERROR', $message);
}

/**
 * Drake action.
 *
 * Execute a shell command.
 */
function drake_action_shell($context) {
  if (!drush_shell_exec_interactive($context['command'])) {
    return drake_action_error(dt('command failed.'));
  }
}

/**
 * Drake action.
 *
 * Execute a drush command.
 */
function drake_action_drush($context) {
  $args = $options = array();
  foreach ($context['args'] as $key => $arg) {
    if (is_int($key)) {
      $args[] = $arg;
    }
    else {
      $options[$key] = $arg;
    }
  }
  $res = drush_invoke_process($context['target'], $context['command'], $args, $options, TRUE);

  if (!$res || $res['error_status'] != 0) {
    return drake_action_error(dt('command failed.'));
  }
}
