<?php

/**
 * Customize blog form
 * 
 * @see woolman_form_alter()
 */
function _woolman_blog_form_alter(&$form, $form_state) {
  global $user;

  // Only privledged users can publish a blog article
  $allowed = array('staff', 'administrator');
  if (!array_intersect($user->roles, $allowed)) {
    $form['field_journal_date']['#access'] = FALSE;
    if (empty($form['nid']['#value'])) {
      $form['field_journal_date'][0]['#default_value']['value'] = NULL;
    }
  }
  // Populate author field
  if (empty($form['field_journal_author'][0]['#default_value']['value'])) {
    $about = '';
    civicrm_initialize();
    $cid = woolman_user_cid();
    $c = woolman_civicrm_api('contact', 'get', array('id' => $cid, 'return.custom_54' => 1));
    if (!empty($c['values'][$cid]['custom_54'])) {
      $about = ', ' . $c['values'][$cid]['custom_54'];
    }
    $form['field_journal_author'][0]['#default_value']['value'] = woolman_name('full') . $about;
  }
  // Remove "headshot" field - we don't use it anymore
  if (empty($form['field_journal_image'][0]['#default_value']['fid'])) {
    $form['field_journal_image']['#access'] = FALSE;
  }
}

/**
 * Handle new blog submission
 * 
 * @see woolman_website_nodeapi()
 */
function _woolman_blog_insert($node) {
  global $user;

  // Tell unprivledged users that their article has not been published yet.
  $allowed = array('staff', 'administrator');
  if (!array_intersect($user->roles, $allowed)) {
    drupal_set_message('Your article has been submitted to our staff for review. We will post it or contact you about it soon. Thanks for your contribution.');
  }

  // Send mail alert to admin
  $message = array('subject' => 'Blog submission by ' . woolman_name('display'));
  $message['body'] = 'Title: ' . $node->title . "\n" .
  'Author: ' . $node->field_journal_author[0]['value'] . "\n" .
  'View entry at '. url('node/' . $node->nid, array('absolute' => TRUE));
  drupal_mail('woolman_website', 'blog', 'webmaster@woolman.org', language_default(), $message, '"' . woolman_name('full') . ' [Woolman Blog]" <' . $user->mail . '>');
}
