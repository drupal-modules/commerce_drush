commerce_drush
==============

Commerce Drush (Drupal module).

Commerce Drush, the Drupal Shell interface, allows you to execute Commerce operations from the command line.

It provides the following commands:

 - commerce-order-add (coa)
 - commerce-order-pay (cop)
 - generate-customer-profiles (gencp)

Run "drush help " to see supported command line options and arguments.

Examples
========

### Create an order (commerce-order-add).

#### Examples:

##### Create an order for product 1.                   

    drush commerce-order-add 1

##### Create an order for product 2 and 3.             

    drush coa 2,3

##### Create an order for 2 products with SKU SKU1001.

    drush --user=admin coa SKU1001 --quantity=2

##### Before placing an order for product, empty the current cart.

    drush --user=admin coa SKU1001 --empty

### Creates full payment for the (cart) order and completes the order (commerce-order-pay):

#### Usage:

    drush --user=USER_NAME_OR_UID commerce-order-pay

#### Examples:

##### Completes (cart) order for the admin user.

    drush --user=admin commerce-order-pay

### Generates customer profiles.                                                                      

#### Usage:

    drush generate-customer-profiles <entity_type>.

#### Examples:

##### Generates customer profiles for all users.                                                                      
    
    drush generate-customer-profiles billing 1
    
##### Replaces all existing admin's customer profiles with a new one.                                                                    
    drush --kill --users=admin generate-customer-profiles billing 1
    
##### Generates customer profiles for all users and skips the prompt for continuation when there are more that 20 users in the database. 
  
    drush --force generate-customer-profiles billing 1
    
##### Generates a single customer profile for admin using specified custom entity type.                               
    
    drush --users admin --entity-type="my-customer-profile-entity-type" billing 1

##### Generates a single customer profile for admin being logged as admin

    drush --user admin --users admin billing 1
    
Dependencies
============
- Commerce
- Drush

Support
=======

Home page:
https://drupal.org/project/commerce_drush

Known bugs:
https://drupal.org/project/issues/commerce_drush

Credits
=======
This project has been sponsored by [Opsview](http://www.opsview.com/).
