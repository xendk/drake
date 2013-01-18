<?php

/**
 * @file
 * Standard actions for Drake.
 */

function drake_standard_actions() {
  return array(
    'shell' => array(
      'callback' => 'drake_action_shell',
    ),
  );
}

function drake_action_error($message) {
  $context = &drush_get_context('DRAKE_RUN');
  $message = dt('@action action failed: ', array('@action' => $context['action']['name'])) . $message;
  return drush_set_error('DRAKE_TASK_ERROR', $message);
}

/**
 * Drake action.
 *
 * Execute a shell command.
 */
function drake_action_shell($context) {
  if (empty($context['command'])) {
    drake_action_error(dt('no command specified.'));
  }

  if (!drush_shell_exec_interactive($context['command'])) {
    drake_action_error(dt('command failed.'));
  }
}
