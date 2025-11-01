/* Minimal jQuery helpers for nav & UX */
$(function () {
  // Highlight current page
  const here = location.pathname.split('/').pop() || 'index.html';
  $('.nav a[data-page="' + here + '"]').addClass('active');
  // External GitHub link opens in new tab
  $('a[target="_repo"]').attr('target', '_blank').attr('rel', 'noopener');
  // Smooth scroll for on-page anchors (if any)
  $('a[href^="#"]').on('click', function (e) {
    const id = $(this).attr('href');
    if (id.length > 1 && $(id).length) {
      e.preventDefault();
      $('html, body').animate({ scrollTop: $(id).offset().top - 70 }, 350);
    }
  });
});