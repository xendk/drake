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

  function testDrakefileDiscovery() {
    // No file should give an error.
    $this->drush('drake 2>&1', array(), array(), NULL, NULL, self::EXIT_ERROR);
    // Check error message.
    $this->assertRegExp('/No drakefile.php files found or specified/', $this->getOutput());

    // A makefile in the current directory should be used.
    copy(dirname(__FILE__) . '/simple.drakefile.php', './drakefile.php');
    $this->drush('drake');
    // Check output.
    $this->assertRegExp('/Simple drakefile\./', $this->getOutput());
    unlink('./drakefile.php');

    // A drakefile.php in sites/all/drush should be used.
    $this->setUpDrupal(1);
    // mkdir($this->webroot() . '/sites/all/drush');
    copy(dirname(__FILE__) . '/site.drakefile.php',
      $this->webroot() . '/sites/all/drush/drakefile.php');

    $this->drush('@dev drake');
    // Check output.
    $this->assertRegExp('/Site drakefile\./', $this->getOutput());

    // Check that drakefile in the current directory is ignored when alias is
    // specified on the commandline.
    copy(dirname(__FILE__) . '/simple.drakefile.php', './drakefile.php');
    $this->drush('@dev drake');
    $this->assertRegExp('/Site drakefile\./', $this->getOutput());
    $this->drush('@dev drake', array('site'));
    // Check output.
    $this->assertRegExp('/Site drakefile\./', $this->getOutput());
    unlink('./drakefile.php');

    // Test that the site drakefile is found when we're in the site dir.
    chdir($this->webroot());
    $this->drush('drake');
    // Check output.
    $this->assertRegExp('/Site drakefile\./', $this->getOutput());

    // Check that a drakefile in the current directory takes precedence over
    // sitewide.
    copy(dirname(__FILE__) . '/simple.drakefile.php', './drakefile.php');
    $this->drush('drake');
    $this->assertRegExp('/Simple drakefile\./', $this->getOutput());
    $this->drush('drake', array('site'));
    // Check output.
    $this->assertRegExp('/Site drakefile\./', $this->getOutput());

    // Test that a drakefile in users home dir is loaded.
    copy(dirname(__FILE__) . '/user.drakefile.php', getenv('HOME') . '/.drush/user.drakefile.drushrc.php');
    $this->drush('drake', array('user'));
    // Check output.
    $this->assertRegExp('/User drakefile\./', $this->getOutput());
    unlink(getenv('HOME') . '/.drush/user.drakefile.drushrc.php');

    // Test that user drakefiles also work in subdirs.
    mkdir(getenv('HOME') . '/.drush/test');
    copy(dirname(__FILE__) . '/user.drakefile.php', getenv('HOME') . '/.drush/test/user.drakefile.drushrc.php');
    $this->drush('drake', array('user'));
    // Check output.
    $this->assertRegExp('/User drakefile\./', $this->getOutput());

    // Take the site drakefile out of the equation.
    unlink($this->webroot() . '/sites/all/drush/drakefile.php');
    // No target should hit the drakefile in the current dir.
    $this->drush('drake');
    $this->assertRegExp('/Simple drakefile\./', $this->getOutput());

    // Remove the drakefile in the current dir. Now only the user drakefile is
    // left.
    unlink('./drakefile.php');
    // No target  should give an error, even when there's user drakefiles.
    $this->drush('drake 2>&1', array(), array(), NULL, NULL, self::EXIT_ERROR);
    // Check error message.
    $this->assertRegExp('/No drakefile.php files found or specified/', $this->getOutput());

    // But specific targets should work.
    $this->drush('drake', array('user'));
    // Check output.
    $this->assertRegExp('/User drakefile\./', $this->getOutput());

    unlink(getenv('HOME') . '/.drush/test/user.drakefile.drushrc.php');
    rmdir(getenv('HOME') . '/.drush/test');
  }

  function testContexts() {
    copy(dirname(__FILE__) . '/context.drakefile.php', './drakefile.php');
    $this->drush('drake', array('simple'));
    // Check output.
    $this->assertRegExp('/simple: value1/', $this->getOutput());

    $this->drush('drake', array('replaced'));
    // Check output.
    $this->assertRegExp('/replaced: value1/', $this->getOutput());

    $this->drush('drake', array('replaced-with-extra'));
    // Check output.
    $this->assertRegExp('/replaced-with-extra: before value1 after/', $this->getOutput());

    $this->drush('drake', array('string-manipulation'));
    // Check output.
    $this->assertRegExp('/string-manipulation: before VALUE1 after/', $this->getOutput());

    $this->drush('drake 2>&1', array('unknown-manipulation'), array(), NULL, NULL, self::EXIT_ERROR);
    // Check output.
    $this->assertRegExp('/Unknown or invalid operation/', $this->getOutput());

    $this->drush('drake', array('optional-context-set'));
    // Check output.
    $this->assertRegExp('/optional-context: Context is set with value: ocs-value/', $this->getOutput());

    $this->drush('drake', array('optional-context-unset'));
    // Check output.
    $this->assertRegExp('/optional-context: Context is not set./', $this->getOutput());

    $this->drush('drake', array('optional-context-set-default'));
    // Check output.
    $this->assertRegExp('/optional-context-default: Context is set with value: ocsd-value/', $this->getOutput());

    $this->drush('drake', array('optional-context-unset-default'));
    // Check output.
    $this->assertRegExp('/optional-context-default: Context is set with value: ocd-default-value/', $this->getOutput());

    // Test that command line definition overrides.
    // First check the set value.
    $this->drush('drake', array('override-context'));
    // Check output.
    $this->assertRegExp('/override-context: Context is set with value: value1/', $this->getOutput());

    // Then try to override.
    $this->drush('drake', array('override-context', 'context1=new-value'));
    // Check output.
    $this->assertRegExp('/override-context: Context is set with value: new-value/', $this->getOutput());

    unlink('./drakefile.php');
  }

  function testArguments() {
    copy(dirname(__FILE__) . '/arguments.drakefile.php', './drakefile.php');

    // Simple argument.
    $this->drush('drake', array('simple', 'testvalue'));
    // Check output.
    $this->assertRegExp('/simple: Context is set with value: testvalue/', $this->getOutput());

    // Simple argument should be required.
    $this->drush('drake 2>&1', array('simple'), array(), NULL, NULL, self::EXIT_ERROR);
    // Check output.
    $this->assertRegExp('/Missing argument: string to print/', $this->getOutput());
    // Optional argument set.
    $this->drush('drake', array('simple-optional', 'testvalue'));
    // Check output.
    $this->assertRegExp('/simple-optional: Context is set with value: testvalue/', $this->getOutput());

    // Optional argument not set.
    $this->drush('drake', array('simple-optional'));
    // Check output.
    $this->assertRegExp('/simple-optional: Context is not set./', $this->getOutput());

    // Optional argument with default  set.
    $this->drush('drake', array('simple-optional-default', 'testvalue'));
    // Check output.
    $this->assertRegExp('/simple-optional-default: Context is set with value: testvalue/', $this->getOutput());

    // Optional argument with default not set.
    $this->drush('drake', array('simple-optional-default'));
    // Check output.
    $this->assertRegExp('/simple-optional-default: Context is set with value: default-value/', $this->getOutput());

    unlink('./drakefile.php');
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
    $this->drush('drake', array('multiple-shell-action-params-keyed'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Slartibartfast/', $this->getOutput());
    $this->assertRegExp('/Deriparamaxx/', $this->getOutput());

    // Test that an array of commands with additional params work.
    $this->drush('drake', array('multiple-shell-action-params-flat'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Slartibartfast/', $this->getOutput());
    $this->assertRegExp('/Deriparamaxx/', $this->getOutput());

    // Failing commands should fail the whole process.
    $this->drush('drake 2>&1', array('failing-shell-action'), $this->options, NULL, NULL, self::EXIT_ERROR);
    $this->assertRegExp('/Action "shell" failed: command failed./', $this->getOutput());
  }

  function testContextActions() {
    $this->drush('drake', array('context-shell-action'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Arthur Dent/', $this->getOutput());

    $this->drush('drake', array('multiple-contexts'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Arthur Dent/', $this->getOutput());
    $this->assertRegExp('/Ford Prefect/', $this->getOutput());

    $this->drush('drake', array('global-context-shell-action'), $this->options);
    // Check for shell command output.
    $this->assertRegExp('/Marvin/', $this->getOutput());

  }

}
