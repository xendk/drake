<?php

/**
 * @file
 * @todo
 */

include_once 'drake.context.inc';
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
      'show-tasks' => 'List available tasks.',
      'show-files' => 'List used drakefiles.',
      'show-context' => 'List the context for the given (or default) task.',
      'ignore-current-dir' => 'Ignore drakefile.php in the current directory',
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
      return dt('Drake');
    case 'meta:drake:summary':
      return dt('Do stuff.');
    case 'drush:drake':
      return dt("Does stuff...");
  }
}

/**
 * Command callback.
 *
 * Executes the main drake command.
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
    // Check current directory if alias was not defined on the command line.
    if (!drush_get_context('DRUSH_TARGET_SITE_ALIAS')) {
      // Use a drakefile file in the current directory.
      if (file_exists('drakefile.php') && !drush_get_option('ignore-current-dir', FALSE)) {
        $files[] = 'drakefile.php';
      }
    }
  }

  if (empty($files)) {
    return drush_set_error('NO_DRAKEFILE_FILE', dt('No drakefile.php files found or specified'));
  }

  $files = array_reverse($files);
  // @todo make this a separate command?
  if (drush_get_option('show-files', FALSE)) {
    drush_print(dt('Loaded drakefiles:'));
    foreach ($files as $file) {
      drush_print($file);
    }
    return;
  }

  $tasks = array();
  $actions = drake_standard_actions();

  $global_context = new DrakeContext();
  // Set up global context.
  $self_record = drush_sitealias_get_record('@self');
  if (!empty($self_record)) {
    // Oddly, it might be '@self'.
    if ($self_record['#name'] == '@self') {
      $global_context['@self'] = '@self';
    }
    else {
      $global_context['@self'] = '@' . $self_record['#name'];
    }
  }

  foreach ($files as $file) {
    // @todo investigate whether this is the most kosher way to include file.
    $context = new DrakeContext($global_context);
    $res = _drake_load_file($file, $context);
    if (!empty($res['tasks'])) {
      $tasks = array_merge($tasks, $res['tasks']);
    }
    if (!empty($res['actions'])) {
      $actions = array_merge($actions, $res['actions']);
    }
  }

  // Add array key as name in sub-arrays..
  _drake_index_array($tasks, 'name');
  _drake_index_array($actions, 'name');

  // @todo make this a separate command?
  if (drush_get_option('show-tasks', FALSE)) {
    drush_print(dt('Available tasks:'));
    drush_print(join(', ', array_keys($tasks)));
    return;
  }

  // Cleanup tasks. Move this to a dedicated function when it gets too
  // complicated.
  foreach ($tasks as $name => $task) {
    // Allow single dependencies to be specified as a string.
    if (isset($task['depends']) && is_string($task['depends'])) {
      $task['depends'] = array($task['depends']);
    }
    $tasks[$name] = $task;
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
  foreach ($run_tasks as &$task) {
    if (!_drake_prepare_task($task, $actions)) {
      return FALSE;
    }
  }
  // Unset reference, else the following foreach loop will munge the last task.
  unset($task);

  if (drush_get_option('show-context', FALSE)) {
    $context = $start_task['#context']->toArray();
    if (!empty($context)) {
      drush_print(dt('Context for the @task "task":', array('@task' => $start_task['name'])));
      $rows = array(array('Name', 'Value'));
      foreach ($context as $name => $value) {
        $rows[] = array($name, $value);
      }
      drush_print_table($rows, TRUE);
    }
    else {
      drush_print(dt('No context variables for "@task" task.', array('@task' => $start_task['name'])));
    }
    return;
  }

  $total_tasks = sizeof($run_tasks);
  $task_num = 1;
  foreach ($run_tasks as $task) {
    try {
      drush_log(_drake_task_message($task) . ' [' . $task_num++ . '/' . $total_tasks . ']', 'status');

      // We need drush_invoke_process to use post for redispatching, as using get
      // will loose our drake command option (as it is an array).
      $backend_options = array(
        'method' => 'POST',
        '#output-label' => $task['name'] . ': ',
      );
      $command_options = drush_redispatch_get_options();
      // $command_options['yes'] = TRUE;
      $task['#context']->resolve($task);
      $command_options['drake'] = array(
        'task' => $task,
        'files' => $files,
      );
      $result = drush_invoke_process('@self', 'drake-process', array($task['name']), $command_options, $backend_options);
      if ($result['error_status'] !== 0) {
        // We assume that _drake_run_task already output a message.
        return FALSE;
      }
      drush_log("Done\n", 'ok');
    }
    catch (DrakeContextException $e) {
      drush_set_error('CONTEXT_MISSING', dt('Task @task: @message', array('@task' => $task['name'], '@message' => $e->getMessage())));
      return FALSE;
    }
    catch (Exception $e) {
      drush_set_error('EXCEPTION_CAUGHT', dt('Task @task threw exception: @message', array('@task' => $task['name'], '@message' => $e->getMessage())));
      return FALSE;
    }
  }
}

/**
 * Command callback to process a single target.
 */
function drush_drake_process($target) {
  $drake = drush_get_option('drake', NULL);

  if (empty($drake['task'])) {
    return drush_set_error('NO_TASK', dt('Task not specified for drake-process.'));
  }

  if (!empty($drake['files'])) {
    // Load the drakefiles to define the functions they contain.
    foreach ($drake['files'] as $file) {
      // @todo investigate whether this is the most kosher way to include file.
      include($file);
    }
  }
  if (_drake_run_task($drake['task']) === FALSE) {
    // If error code isn't set, set a generic error message to ensure that our
    // return code is failure..
    if (drush_get_error() == DRUSH_SUCCESS) {
      return drush_set_error('UNKNOWN_ERROR', dt('Task failed with an unknown error.'));
    }
    // Else assume that the task set the message.
    return FALSE;
  }
}

/**
 * Load a drakefile, adding context.
 *
 * Adds context objects to tasks and actions and saves file scope context.
 */
function _drake_load_file($_drake_file, $_drake_context) {
  include $_drake_file;
  $return = array();
  if (!empty($tasks)) {
    foreach ($tasks as &$task) {
      $task['#context'] = $_drake_context;
    }
    $return['tasks'] = $tasks;
  }
  if (!empty($actions)) {
    foreach ($actions as &$action) {
      $action['#context'] = $_drake_context;
    }
    $return['actions'] = $actions;
  }
  if (!empty($context)) {
    foreach($context as $key => $value) {
      $_drake_context[$key] = $value;
    }
  }
  return $return;
}

/**
 * Return the message for the task.
 */
function _drake_task_message($task) {
  if (!empty($task['message'])) {
    return $task['message'];
  }
  else {
    return dt('Doing "@task_name" task', array('@task_name' => $task['name']));
  }
}

/**
 * Check and prepare a task.
 *
 * Ensures that actions are properly configured, handles multiple actions and
 * action parameters.
 */
function _drake_prepare_task(&$task, &$actions) {
  if (!empty($task['action'])) {
    if (!isset($actions[$task['action']])) {
      return drush_set_error("UNKNOWN_ACTION", dt('Unknown action "@action".', array('@action' => $task['action'])));
    }

    $action = $actions[$task['action']];
    $task['action'] = $action;

    if (empty($action['callback']) ||
      !function_exists($action['callback'])) {
      return drush_set_error("UNKNOWN_ACTION_CALLBACK", dt('Callback for action "@action" does not exist.', array('@action' => $action['name'])));
    }

    $action += array(
      'multiple' => '',
      'parameters' => array(),
    );
    // Compile default parameters.
    $action_defaults = array();
    foreach ($action['parameters'] as $param_name => $param_settings) {
      if (is_array($param_settings) && (array_key_exists('default', $param_settings))) {
        $action_defaults[$param_name] = $param_settings['default'];
      }
    }
    // Check the action parameters and add in the action(s) on the task.
    $muliple_parm = $action['multiple'];
    if ($muliple_parm && isset($task[$muliple_parm . 's'])) {
      if (!empty($task[$muliple_parm])) {
        return drush_set_error('AMBIGUOUS_PARAM', dt('Action "@action" failed: Both "@singular_parm" and "@plural_parm" cannot be specified.', array('@action' => $action['name'], '@singular_parm' => $muliple_parm, '@plural_parm' => $muliple_parm . 's')));
      }
      $task_actions = array();
      // Explode the multiple parameter to multiple actions.
      foreach ($task[$muliple_parm . 's'] as $key => $params) {
        // If the key is int, use the value as command.
        if (is_int($key)) {
          // If the params isn't an array, assume it's a simple command string.
          if (!is_array($params)) {
            $params = array($muliple_parm => $params);
          }
        }
        else {
          // Else use the key as the command.
          $params += array($muliple_parm => $key);
        }
        $task_actions[] = $params + $task + $action_defaults;
      }
    }
    else {
      $task_actions = array($task + $action_defaults);
    }

    // Check that required parameters is present.
    foreach ($task_actions as $task_action) {
      $diff = array_diff(array_keys($action['parameters']), array_keys($task_action));
      if (!empty($diff)) {
        return drush_set_error('MISSING_PARAM', dt('Action "@action" failed: Required param(s) not supplied: "@params".', array('@action' => $action['name'], '@params' => join(', ', $diff))));
      }
    }

    $task['#actions'] = $task_actions;
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
function _drake_run_task($task) {
  if (isset($task['#actions'])) {
    foreach ($task['#actions'] as $run_action) {
      $function = $run_action['action']['callback'];
      $context = &drush_get_context('DRAKE_RUN');
      $context['action'] = $run_action['action'];
      $context['task'] = $task;
      if ($function($run_action) === FALSE) {
        $context = array();
        return drush_set_error('TASK_FAILED', dt('Task "@task" failed, aborting.', array('@task' => $task['name'])));
      }
    }
  }
}

/**
 * Resolve task dependencies by doing a depth first search.
 */
function drake_resolve_dependencies(&$task, &$tasks) {
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
function _drake_visit(&$task, &$resolution, &$all_tasks, &$seen, $stack) {
  $seen_id = $task['name'] . $task['#context']->md5();
  if (!isset($seen[$seen_id])) {
    $seen[$seen_id] = TRUE;
    $context = $task['#context'];
    if (isset($task['context'])) {
      $context = new DrakeContext($context);
      foreach ($task['context'] as $name => $value) {
        $context[$name] = $value;
      }
      // Set the new context.
      $task['#context'] = $context;
    }
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
        // Let context follow dependencies.
        $dependency = $all_tasks[$dependency_name];
        $dependency['#context'] = new DrakeContext(array($dependency['#context'], $context));
        $res = _drake_visit($dependency, $resolution, $all_tasks, $seen, array_merge($stack, array($task['name'])));
        if (!$res) {
          return $res;
        }
      }
    }
    $resolution[] = $task;
  }
  return TRUE;
}
