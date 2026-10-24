.. _developer_reg_test:

Regression Tests
================

Regression tests are used to ensure that ArcXpress is

- functionally correct
- no degradation in *Quality-of-Results* (QoR)

Regression tests are required to pass before merging any code changes.

Run tests
---------

Regression tests can be run through the following command at the root directory of the project 

.. code-block::

  make reg_test

.. note:: To accelerate runtime, you may customize the number of jobs by adding ``NUM_JOBS=<int>`` to make command


Add a New Testcase
------------------

To add a new test case to regression test, a number of steps should be followed:

- Create your test case under ``test/tasks``. Please name your testcase in a reasonable ways, so that other developers can identify its usage easily. For example, ``test/tasks/post_synth`` is to validate the support of post-synthesis projects.
- A test case should include at least the following files

  - A script to call ArcXpress. See example, ``test/tasks/post_synth/config/run.arkangel``.
  - A project file required by the ArcXpress script. See example, ``test/tasks/post_synth/config/project.xml``.

- Add the testcase to the task list under ``test/common/task_list.yaml``.

Debug tests
-----------

If a regression test failed, 

- detailed logs and intermediate files can be found under a ``_work`` directory. For example, ``test/tasks/post_synth/_work``.
- test run can be reproduced by calling the command in ``_work/run.arkangel_runcmd.sh``. Remeber that the command should be called under the ``_work`` directory
