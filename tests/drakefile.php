<?php

$tasks['recur1'] = array(
  'depends' => array('recur2'),
);

$tasks['recur2'] = array(
  'depends' => array('recur3'),
);

$tasks['recur3'] = array(
  'depends' => array('recur1'),
);

$tasks['actionless'] = array(
  'action' => 'unknown', // Undefined action.
);

$tasks['task-with-working-action'] = array(
  'action' => 'good-action',
);

$tasks['task-with-failing-action'] = array(
  'action' => 'failing-action',
);

$tasks['shell-action'] = array(
  'action' => 'shell',
  'command' => 'echo "Slartibartfast"',
);

$tasks['failingshell-action'] = array(
  'action' => 'shell',
  // The Unix false command which returns a non-zero exit code.
  'command' => 'false',
);

$actions['good-action'] = array(
  'callback' => 'test_good_action',
);

$actions['failing-action'] = array(
  'callback' => 'test_failing_action',
);

function test_good_action($context) {
  drush_print('Test..');
}

function test_failing_action($context) {
  return drush_set_error('NOGOOD', dt('Arg, failure.'));
}
