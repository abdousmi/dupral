This modules set provides out of box plugged system to Monext Payline Payment Service.
You can find description about this Payment Provider at http://www.payline.com/.
The payment methods implemented are
 1/ Payline Web Payment (offsite method and payment on order)

--------
FEATURES
--------
* Full integred plugged system to Monext Payline Web Payment (offsite payment / payment on order)
* Full Payline Settings Form (proxy, contracts and customisation settings)
* Primary and secundary contracts handler
* Settings payline validation by web service on settings submit
* Primary contrats choice in checkout process
* Full information from payline in drupal commerce transactions dashboard
* Direct link to payline dashboard from drupal commerce transactions dashboard
* Partial or full refund from drupal commerce transactions dashboard
  (with line items handler in refund form or opportunity to refund a free amount)
  this feature can be disabled with a "commerce_paylinebymonext_ui_refund_disabled" variable you can set in a custom module
  @see commerce_paylinebymonext.api.php
* Support for payment on expedition
* Support to give information about fraud suspection direct on order (you need to add rules to do that)
* Add rules pluggin :
** Condition : Transaction status is
** Event : Connexion error with Payline server (can help to add specific status on command if this connexion
           problem happens
* WS support for contracts number automatics complemetion

-------
ROADMAP
-------


-------------------------
INSTALL AND CONFIGURATION
-------------------------
1/ Download the commerce_paylinebymonext module archive and extract it in your contrib modules folder.
2/ Enable the payment method module(s) of your choice.
   Currently two method are availables are available :
    * commerce_paylinebymonext_wp (Web Payment) : A simple method to do web payment on checkout customer
    * commerce_paylinebymonext_sa_wp (Web Payment with simple auth)  : Monext Payline Web Payment does a simple auth on
      checkout and do capture for payment only when rules you can configured are fired. By default one default rules
      fires capture when order status is setted to completed (handly as automatically)
3/ Go to admin/commerce/config/payment-methods
    * Enable the payment methods of your choice
    * Edit It
    * Edit the Enable payment method Action Element
4/ Fill your access settings, your payline environnement (homologation or production)
5/ If your hosting provider external acces is under a proxy, set your proxy settings
6/ Set your primary and secondary contracts with the contract number got from Payline
7/ Set your customisation settings
8/ Save and enjoy !

---------------------------------------------------------------------------
details about commerce_paylinebymonext_sa_wp (Web Payment with simple auth)
---------------------------------------------------------------------------
This method do a simple auth during checkout (7 days valid).
A rules action is provided by module to fire the capture (you can use it in your custom rules if you need specific
behaviour)
By default, a rules is enable : it fires capture for payment when the status order get the "completed" status.
 * Order status are managed handly from the order administration panel (admin/commerce/orders)
 * Order status can also be managed programmaticaly or by rules (ie. you can have a rules that set the order status when
   your site is informed by a webservice that your logistic plateform send the products)

 If you want just change this default rules you can
 * go to admin/config/workflow/rules path
 * edit (or clone it if you want keep the default rules) Payline order authorized rules
 * you can add event, conditions or actions (know how use rules is required to do it)
 * if you just want change the order status matching to fire capture
   ** edit the condition "data comparaison. Parameter: Data to compare: [commerce-order:status], Data value: Completed"
   ** change the Data value

-----------------------------------------------------------------
How add rules to give information on order about fraud suspection
-----------------------------------------------------------------
* Go to admin/config/workflow/rules path
* Create a new rule
* add an event : After saving a new commerce payment transaction (available in Commerce Payment Section)
* add an event : After After updating an existing commerce payment transaction (available in Commerce Payment Section)
* add a condition : Transaction status is (available in Payline section)
* set the value field of this condition on : Parameter: Transaction: [commerce_payment_transaction], Status: Fraud suspected / transaction ok
* add a condition : Transaction status is (available in Payline section)
* set the value field of this condition on : Parameter: Transaction: [commerce_payment_transaction], Status: Fraud suspected / transaction ko
* you have to put this two conditions in a or group
* add action : Update the order status (available in Commerce Order)
* set Data selector to "commerce-payment-transaction:order"
* set value to fraud suspected


---------------------------------
API AND DEVELOPMENT CUSTOMISATION
---------------------------------
* All hook and accessible variables are described in commerce_paylinebymonext.api.php file

------------
KNOWW ISSUES
------------
404 error on payment from Payline plateform return :
 * cache menu need to be rebuild.
 * You can do it with one of the following solutions :
    * with drush : drush cc menu
    * with admin menu module (home > flush all caches > menu
    * if you don't have neither drush or admin menu module :
        * go to admin/config/development/performance path
        * use Clear all caches button
