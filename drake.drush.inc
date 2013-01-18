<?php

/**
 * @file
 * @todo
 */

include_once 'drake.actions.inc';

/**
 * Implements hook_drush_command().
 */
function drake_drush_command() {
  $items['drake'] = array(
    'description' => 'Do something.',
    'arguments' => array(
      'target' => 'Task to make.',
    ),
    'options' => array(
      'file' => 'Use specified file.',
      'list' => 'List available tasks.',
      'list-files' => 'List used drakefiles.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  // Hidden command to make a single task.
  $items['drake-process'] = array(
    'hidden' => TRUE,
    'arguments' => array(
      'target' => 'The target to build.',
    ),
    'options' => array(
      'drake' => 'The collected tasks and actions.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

/**
 * Implements hook_drush_help().
 */
function drake_drush_help($section) {
  switch ($section) {
    case 'meta:drake:title':
      return dt('Do');
    case 'meta:drake:summary':
      return dt('Do stuff.');
    case 'drush:drake':
      return dt("Does stuff...");
  }
}

/**
 * Command callback.
 */
function drush_drake($target = '') {
  // Drake files to load.
  $files = array();

  $file = drush_get_option('file', NULL);
  if (!empty($file)) {
    // A file was specified, use that.
    if (!file_exists($file)) {
      return drush_set_error('MISSING_FILE', dt('File @file not found.', array('@file' => $file)));
    }
    // Override files.
    $files = array($file);
  }
  else {
    // Else look around.
    // Add sites/all/drush if we have a boostrapped site.
    $drupal_root = drush_get_context('DRUSH_SELECTED_DRUPAL_ROOT');
    if (!empty($drupal_root)) {
      $site_all = drush_get_context('DRUSH_SELECTED_DRUPAL_SITES_ALL_DRUSH') . '/drakefile.php';
      if (file_exists($site_all)) {
        $files[] = $site_all;
      }
    }

    // Use a drakefile file in the current directory.
    if (file_exists('drakefile.php')) {
      $files[] = 'drakefile.php';
    }
  }

  if (empty($files)) {
    return drush_set_error('NO_DRAKEFILE_FILE', dt('No drakefile.php files found or specified'));
  }

  // @todo make this a separate command?
  if (drush_get_option('list-files', FALSE)) {
    drush_log(dt('Loaded drakefiles::'), 'status');
    foreach ($files as $file) {
      drush_log($file, 'status');
    }
    return;
  }

  $actions = drake_standard_actions();

  foreach ($files as $file) {
    // @todo investigate whether this is the most kosher way to include file.
    include($file);
  }

  // Add array key as name in sub-arrays..
  _drake_index_array($tasks, 'name');
  _drake_index_array($actions, 'name');

  // @todo make this a separate command?
  if (drush_get_option('list', FALSE)) {
    drush_log(dt('Available tasks:'), 'status');
    drush_log(join(', ', array_keys($tasks)), 'status');
    return;
  }

  // Find the main task we're doing.
  if (!empty($target)) {
    if (!isset($tasks[$target])) {
      return drush_set_error('UNKNOWN_TASK', dt('Unknown task @task', array('@task' => $target)));
    }
    $start_task = $tasks[$target];
  }
  else {
    $start_task = reset($tasks);
  }

  // Add in dependent tasks.
  if (!($run_tasks = drake_resolve_dependencies($start_task, $tasks))) {
    return FALSE;
  }

  // Check that we can run the given tasks.
  foreach ($run_tasks as $delta => $task) {
    if (!_drake_check_task($task, &$actions)) {
      return FALSE;
    }
  }

  foreach ($run_tasks as $delta => $task) {
    drush_log(_drake_task_message($task), 'status');
    // We need drush_invoke_process to use post for redispatching, as using get
    // will loose our drake command option (as it is an array).
    $backend_options = array(
      'method' => 'POST',
    );
    $command_options = drush_redispatch_get_options();
    // $command_options['yes'] = TRUE;
    $command_options['drake'] = array(
      'tasks' => $tasks,
      'actions' => $actions,
      'files' => $files,
    );
    $result = drush_invoke_process('@self', 'drake-process', array($task['name']), $command_options, $backend_options);
    if ($result['error_status'] !== 0) {
      return drush_set_error('TASK_FAILED', dt('Task "@task" failed, aborting.', array('@task' => $task['name'])));
    }
  }
}

/**
 * Command callback to process a single target.
 */
function drush_drake_process($target) {
  $drake = drush_get_option('drake', NULL);

  if (empty($drake['tasks']) || empty($drake['actions'])) {
    return drush_set_error('BAD_ARGUMENTS', dt('Bad arguments for drake-process.'));
  }

  if (isset($drake['tasks'][$target])) {
    if (!empty($drake['files'])) {
      // Load the drakefiles to define the functions they contain.
      foreach ($drake['files'] as $file) {
        // @todo investigate whether this is the most kosher way to include file.
        include($file);
      }
    }
    if (_drake_run_task($drake['tasks'][$target], $drake['actions']) === FALSE) {
      // If error code isn't set, set a generic error message to ensure that our
      // return code is failure..
      if (drush_get_error() == DRUSH_SUCCESS) {
        return drush_set_error('UNKNOWN_ERROR', dt('Task failed with an unknown error.'));
      }
      // Else assume that the task set the message.
      return FALSE;
    }
  }
  else {
    return drush_set_error('UNKNOWN_TASK', dt('Unknown task @task', array('@task' => $target)));
  }
}

/**
 * Return the message for the task.
 */
function _drake_task_message($task) {
  if (!empty($task['message'])) {
    return $task['message'];
  }
  else {
    return dt('Doing "@task_name" task.', array('@task_name' => $task['name']));
  }
}

/**
 * Check that a task can be run.
 */
function _drake_check_task($task, $actions) {
  if (!empty($task['action'])) {
    if (!isset($actions[$task['action']])) {
      return drush_set_error("UNKNOWN_ACTION", dt('Unknown action "@action".', array('@action' => $task['action'])));
    }
    elseif (empty($actions[$task['action']]['callback']) ||
      !function_exists($actions[$task['action']]['callback'])) {
      return drush_set_error("UNKNOWN_ACTION_CALLBACK", dt('Callback for action "@action" does not exist.', array('@action' => $task['action'])));
    }
  }
  return TRUE;
}

/**
 * Add the key of the array items as the given value in the sub-array.
 */
function _drake_index_array(&$array, $prop) {
  foreach ($array as $key => $item) {
    $array[$key][$prop] = $key;
  }
}

/**
 * Runs a single task.
 */
function _drake_run_task($task, $actions) {
  if (isset($task['action'])) {
    $action = $actions[$task['action']];
    $function = $action['callback'];
    $context = &drush_get_context('DRAKE_RUN');
    $context['action'] = $action;
    $context['task'] = $task;
    if ($function($task) === FALSE) {
      $context = array();
      return drush_set_error('TASK_FAILED', dt('Task "@name" failed.', array('@name' => $task['name'])));
    }
  }
}

/**
 * Resolve task dependencies by doing a depth first search.
 */
function drake_resolve_dependencies($task, $tasks) {
  $resolution = array();
  $seen = array();
  if (!_drake_visit($task, $resolution, $tasks, $seen, array())) {
    return FALSE;
  }
  return $resolution;
}

/**
 * Helper for drake_resolve_dependencies().
 */
function _drake_visit($task, &$resolution, &$all_tasks, &$seen, $stack) {
  if (!isset($seen[$task['name']])) {
    $seen[$task['name']] = TRUE;
    if (!empty($task['depends'])) {
      foreach ($task['depends'] as $dependency_name) {
        // Circular detection.
        if (in_array($dependency_name, $stack)) {
          $chain = join(' -> ', array_merge($stack, array($task['name'], $dependency_name)));
          drush_log(dt('Failing dependency chain: @chain.', array('@chain' => $chain)), 'notice');
          return drush_set_error('RECURSIVE_DEPENDENCY', dt('Recursive dependency'));
        }
        if (!isset($all_tasks[$dependency_name])) {
          return drush_set_error('UNKNOWN_TASK', dt('Unknown task @task', array('@task' => $dependency_name)));
        }
        $res = _drake_visit($all_tasks[$dependency_name], $resolution, $all_tasks, $seen, array_merge($stack, array($task['name'])));
        if (!$res) {
          return $res;
        }
      }
    }
    $resolution[$task['name']] = $task;
  }
  return TRUE;
}
