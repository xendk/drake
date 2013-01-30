<?php

$context = array(
  'context1' => 'value1',
  'context2' => 'value2',
  'context3' => 'value3',
);

$tasks['simple'] = array(
  'action' => 'print',
  'output' => context('context1'),
);

$tasks['replaced'] = array(
  'action' => 'print',
  'output' => context('[context1]'),
);

$tasks['replaced-with-extra'] = array(
  'action' => 'print',
  'output' => context('before [context1] after'),
);

$tasks['string-manipulation'] = array(
  'action' => 'print',
  'output' => context('before [context1:string:upcase] after'),
);

$tasks['unknown-manipulation'] = array(
  'action' => 'print',
  'output' => context('before [context1:upcase] after'),
);

$tasks['test'] = array(
  'action' => 'print',
  'output' => context('@self:site:root'),
);

$actions['print'] = array(
  'callback' => 'drake_print',
);

function drake_print($context) {
  drush_print($context['output']);
}
