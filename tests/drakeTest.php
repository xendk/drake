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
    // Hack for outputting stuff: fwrite(STDOUT, "test\n");

    $this->drush('drake', array('recur1'), $this->options, NULL, NULL, self::EXIT_ERROR);
  }

  function testMissingAction() {
    $this->drush('drake', array('actionless'), $this->options, NULL, NULL, self::EXIT_ERROR);
  }

  function testBasicActions() {
    $this->drush('drake', array('task-with-working-action'), $this->options);
    $this->drush('drake', array('task-with-failing-action'), $this->options, NULL, NULL, self::EXIT_ERROR);
  }

  function testShellAction() {
    $this->drush('drake', array('shell-action'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Slartibartfast/', $this->getOutput());

    // Failing commands should fail the whole process.
    $this->drush('drake', array('failing-shell-action'), $this->options, NULL, NULL, self::EXIT_ERROR);

  }
}
