<?php
/**
 * Callback for ~blog/submissions page
 */
function cust_wool_blog_submission() {
  global $user;

  //privledged users will bypass this step and go right to the blog form
  $allowed = array('staff', 'administrator');
  if (array_intersect($user->roles, $allowed)) {
    drupal_goto('node/add/blog-entry');
  }

  $intro = '<p><strong>We welcome your submissions to the Woolman Blog. Writings, reflections, poetry and art are all wonderful additions to our community.</strong></p>';

  //anonymous users must login first
  if (in_array('anonymous user', $user->roles)) {
    return $intro.'<p><em>In order to proceed, please login, or if you don\'t yet have an account, take a few moments to '.l('register', 'user/register', array('query' => array('destination' => '~blog/submissions'), 'alias' => TRUE)).'.</em></p>';
  }
  else {
    return $intro . drupal_get_form('cust_wool_blog_form');
  }
}

/**
 * Generate blog form
 */
function cust_wool_blog_form($form_state) {
  global $user;
  $about = '';
  civicrm_initialize();
  $cid = woolman_user_cid();
  $c = woolman_civicrm_api('contact', 'get', array('id' => $cid, 'return.custom_54' => 1));
  if (!empty($c['values'][$cid]['custom_54'])) {
    $about = ', ' . $c['values'][$cid]['custom_54'];
  }
  $form = array();
  $form['#prefix'] = '';
  $form['title'] = array('#type' => 'textfield',
                          '#title' => t('Title'),
                          '#required' => TRUE,
                        );
  $form['author'] = array('#type' => 'textfield',
                          '#title' => t('Author'),
                          '#description' => t('Name and description of the person writing this article'),
                          '#required' => TRUE,
                          '#default_value' => woolman_name('full') . $about,
                          );
  $form['body'] = array('#type' => 'textarea',
                          '#title' => t('Body'),
                          '#rows' => 22,
                          '#required' => TRUE,
                        );
  $form['format'] = filter_form();
  $form['submit'] = array('#type' => 'submit',
                            '#value' => t('Submit'),
                            '#submit' => array('cust_wool_blog_submit'),
                          );
  return $form;
}

/**
 * Callback for blog form submission
 */
function cust_wool_blog_submit($form, &$form_state) {
  global $user;
  module_load_include('inc', 'node', 'node.pages');
  $node = new stdClass();
  $node->type = 'blog_entry';
  node_object_prepare($node);

  $node->title = $form_state['values']['title'];
  $node->body = $form_state['values']['body'];
  $node->revision = FALSE;
  $node->field_journal_author = array(array('value' => $form_state['values']['author']));

  node_save($node);

  //get rid of path alias, since it's junk without field_journal_date
  path_set_alias($path = 'node/' . $node->nid);

  drupal_set_message('Thanks for submitting an article to the Woolman Blog! We will review your article and post it or contact you about it soon.');

  // Send mail alert to admin
  $message = array('subject' => 'Blog submission by ' . woolman_name('display'));
  $message['body'] = 'Title: ' . $form_state['values']['title'] . "\n" .
  'Author: ' . $form_state['values']['author'] . "\n" .
  'View entry at http://woolman.org/node/' . $node->nid;
  drupal_mail('woolman_website', 'blog', 'webmaster@woolman.org, grace.oedel@gmail.com', language_default(), $message, '"' . woolman_name('full') . '" <' . $user->mail . '>');

  drupal_goto('~blog');
}
