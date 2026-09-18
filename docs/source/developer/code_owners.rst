.. _developer_code_owners:

Code Ownership
==============

Work Domains
------------
In general, the project can be split into the following work domains

- *Graphical User Interface* (GUI)
- EDA engines (core)

For each work domain, there are explicit subdirectories at the root. Input files and scripts (for regression tests) should be placed in these subdirectories.

Team Responsibilities
---------------------

.. note:: All the team members should be responsive on any github issue and pull request where they are mentioned. Expected response window is in 24 hours. Otherwise, meetings will be forced to synchronize. Meeting notes will be added to related issues and pull requests

GUI team
^^^^^^^^

GUI team is in charge of 

  - Develop, maintain and test graphical user interfaces
  - Interface GUI with EDA commands available in underlying engines
  - Communicate with EDA team when there is a conflict

EDA team
^^^^^^^^

EDA team is in charge of 

  - Develop, maintain and test EDA engines
  - Release EDA commands to GUI team
  - Create and maintain regular releases of the project
  - Communicate with GUI team when there is a conflict

IT team
^^^^^^^

IT team is in charge of 
  - Github Action workflows to enable all the regression tests
  - Resolve any license and environment setup for other teams
