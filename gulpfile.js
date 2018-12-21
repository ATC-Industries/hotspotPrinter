var gulp = require('gulp');
var nunjucksRender = require('gulp-nunjucks-render');

gulp.task('njk', function() {
  // Gets .html and .nunjucks files in pages
  return gulp.src('HotSpotServer/app/pages/**/*.+(html|nunjucks|njk)')
  // Renders template with nunjucks
  .pipe(nunjucksRender({
      path: ['HotSpotServer/app/templates']
    }))
  // output files in app folder
  .pipe(gulp.dest('HotSpotServer/data'))
});
