Drush Drake
===========

Yet another site-building/rebuilding/stuff-doing tool for Drush, this
one taking much more inspiration from the likes of make(1) (and rake,
pake, phake, ant, phing, etc), and other more or less general purpose
building tools, but with Drush Flavour (TM).

Intro
=====

Drush Drake works sorta like make by having a drakefile.php file which
tells it what to do. The drakefile.php may be in the current directory
or in sites/all/drush of the current site.

A drakefile.php file looks a lot like a Drush aliases.drushrc.php
file, and contains tasks and, optionally, actions, in the variables
$tasks and $actions, respectively.

An action is the implementation of something to do, such as syncing a
database or running a Drush command. At the moment the selection is
rather bare, but an healthy selection of standard things you might want
to do with a Drupal site is planned.

Tasks specify which actions you want to run, how, and in which
order. Tasks is much like the targets of traditional make(1), as
they're the main thing you'll be using on a daily basis.

When invoking Drush Drake without any arguments, it will run the first
task found in the file. Providing the name of a task will run that
specific task.

A task can be dependent on other tasks by using the 'depends' key,
which will make Drush Drake run the depended upon tasks before running
the given task.

An example:
@todo
<?php

$tasks['default'] = array(
  'depends' => array('first-task', 'second-task'),
  'action' => 'shell',
  'command' => 'echo "first-task and second-task is now done."',
);

$tasks['first-task'] = array(
  'action' => 'shell',
  'command' => 'echo "First task!"',
);

$tasks['second-task'] = array(
  'action' => 'drush',
  'command' => 'php-eval',
  'args' => array('drush_print("Second task!")'),
);

?>

With the above drakefile.php, running drush drake with no arguments
will run the tasks depended on by the default task before running the
default task itself.

Contexts
========

As running tasks with hard coded values rather limits the reusability of
tasks, we have contexts. They allows you to pass arguments to tasks, and follows the dependency chain. 

<?php

$tasks['echo-task'] = array(
  'action' => 'shell',
  'command' => 'echo',
  'args' => array(context('echo-string')),
);

$tasks['do-echo'] = array(
  'depends' => 'echo-task',
  'context' => array(
    'echo-string' => 'Hello world!',
  ),
);

?>

When the echo-task runs as a dependency of do-echo, the echo-string
context is set to "Hello world!". If do-echo didn't have that context,
the file context of the file that contained the echo-task would be
checked (these can be defined by having a $context variable in the
drakefile), before checking the global context.

Drush Drake sets the context @self to the name of the alias of the
current site (if any).
