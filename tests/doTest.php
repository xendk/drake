<?php

/**
 * @file
 * PHPUnit Tests for Situs.
 */

/**
 * Situs testing class.
 */
class doCase extends Drush_CommandTestCase {

  public static function setUpBeforeClass() {
    parent::setUpBeforeClass();
    // Copy in the command file, so the sandbox can find it.
    copy(dirname(dirname(__FILE__)) . '/do.drush.inc', getenv('HOME') . '/.drush/do.drush.inc');
  }

  // public function setUp() {
  //   if (!empty($this->aliases)) {
  //     return;
  //   }
  //   // Make sure the parent build dir exists.
  //   if (!file_exists($this->webroot())) {
  //     mkdir($this->webroot());
  //   }
  //   // Create an alias.
  //   $this->aliases = array(
  //     'homer' => array(
  //       'root' => $this->webroot() . '/homer',
  //       'make-file' => dirname(__FILE__) . '/simple.make',
  //     ),
  //     'marge' => array(
  //       'root' => $this->webroot() . '/marge',
  //       'make-file' => dirname(__FILE__) . '/simple.make',
  //     ),
  //     'bart' => array(
  //       'root' => $this->webroot() . '/bart',
  //       'make-file' => dirname(__FILE__) . '/with_git.make',
  //     ),
  //     'lisa' => array(
  //       'root' => $this->webroot() . '/lisa',
  //       'make-file' => dirname(__FILE__) . '/failing.make',
  //     ),
  //     'maggie' => array(
  //       'root' => $this->webroot() . '/maggie',
  //       'make-file' => dirname(__FILE__) . '/simple.make',
  //     ),
  //   );

  //   $this->saveAliases();
  // }

  // protected function saveAliases() {
  //   file_put_contents(UNISH_SANDBOX . '/etc/drush/aliases.drushrc.php', $this->file_aliases($this->aliases));
  // }

  function testRecursiveFail() {
    // Hack for outputting stuff: fwrite(STDOUT, "test\n");

    $this->drush('do', array('recur1'), array('file' => dirname(__FILE__) . '/do.php'), NULL, NULL, self::EXIT_ERROR);
  }

  function testMissingAction() {
    $this->drush('do', array('actionless'), array('file' => dirname(__FILE__) . '/do.php'), NULL, NULL, self::EXIT_ERROR);
  }

  function testBasicActions() {
    $this->drush('do', array('task-with-working-action'), array('file' => dirname(__FILE__) . '/do.php'));
    $this->drush('do', array('task-with-failing-action'), array('file' => dirname(__FILE__) . '/do.php'), NULL, NULL, self::EXIT_ERROR);
  }

}
