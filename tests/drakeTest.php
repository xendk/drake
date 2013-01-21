<?php

/**
 * @file
 * PHPUnit Tests for Situs.
 */

/**
 * Situs testing class.
 */
class DrakeCase extends Drush_CommandTestCase {

  public static function setUpBeforeClass() {
    parent::setUpBeforeClass();
    // Copy in the command file, so the sandbox can find it.
    copy(dirname(dirname(__FILE__)) . '/drake.drush.inc', getenv('HOME') . '/.drush/drake.drush.inc');
    copy(dirname(dirname(__FILE__)) . '/drake.actions.inc', getenv('HOME') . '/.drush/drake.actions.inc');
    copy(dirname(dirname(__FILE__)) . '/drake.context.inc', getenv('HOME') . '/.drush/drake.context.inc');
  }

  public function setUp() {
    // Specify file explicitly.
    $this->options = array('file' => dirname(__FILE__) . '/drakefile.php');

    // if (!empty($this->aliases)) {
    //   return;
    // }
    // // Make sure the parent build dir exists.
    // if (!file_exists($this->webroot())) {
    //   mkdir($this->webroot());
    // }
    // // Create an alias.
    // $this->aliases = array(
    //   'homer' => array(
    //     'root' => $this->webroot() . '/homer',
    //     'make-file' => dirname(__FILE__) . '/simple.make',
    //   ),
    // );

  //   $this->saveAliases();
  }

  // protected function saveAliases() {
  //   file_put_contents(UNISH_SANDBOX . '/etc/drush/aliases.drushrc.php', $this->file_aliases($this->aliases));
  // }

  function testRecursiveFail() {
    $this->drush('drake 2>&1', array('recur1'), $this->options, NULL, NULL, self::EXIT_ERROR);
    // Check error message.
    $this->assertRegExp('/Recursive dependency/', $this->getOutput());
  }

  function testStringDependency() {
    $this->drush('drake', array('string-dependency'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Slartibartfast/', $this->getOutput());
  }

  function testBadActions() {
    // No action or depends are useless, but not considered an error.
    $this->drush('drake', array('actionless'), $this->options);

    $this->drush('drake 2>&1', array('unknown-action'), $this->options, NULL, NULL, self::EXIT_ERROR);
    $this->assertRegExp('/Unknown action "unknown"./', $this->getOutput());

    $this->drush('drake 2>&1', array('unknown-action-callback'), $this->options, NULL, NULL, self::EXIT_ERROR);
    $this->assertRegExp('/Callback for action "bad-callback" does not exist./', $this->getOutput());
  }

  function testBasicActions() {
    $this->drush('drake', array('task-with-working-action'), $this->options);

    $this->drush('drake 2>&1', array('task-with-failing-action'), $this->options, NULL, NULL, self::EXIT_ERROR);
  }

  function testShellAction() {
    $this->drush('drake', array('shell-action'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Slartibartfast/', $this->getOutput());

    // Missing params should fail the command.
    $this->drush('drake 2>&1', array('bad-arg-shell-action'), $this->options, NULL, NULL, self::EXIT_ERROR);
    $this->assertRegExp('/Action "shell" failed: Required param\(s\) not supplied: "command"./', $this->getOutput());

    // Check that an simple array of commands work.
    $this->drush('drake', array('multiple-shell-action-simple'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Slartibartfast/', $this->getOutput());
    $this->assertRegExp('/Deriparamaxx/', $this->getOutput());

    // Test that an array of commands with additional params work.
    $this->drush('drake', array('multiple-shell-action-params'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Slartibartfast/', $this->getOutput());
    $this->assertRegExp('/Deriparamaxx/', $this->getOutput());

    // Failing commands should fail the whole process.
    $this->drush('drake 2>&1', array('failing-shell-action'), $this->options, NULL, NULL, self::EXIT_ERROR);
    $this->assertRegExp('/Action "shell" failed: command failed./', $this->getOutput());

  }
}
