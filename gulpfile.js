/*
 
ESP8266 file system builder
 
Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>;
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

5/21/18 : Jason Brown - adapted to operate on multiple html files
*/
 
// -----------------------------------------------------------------------------
// File system builder
// -----------------------------------------------------------------------------
 
const fs = require('fs');
const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const cleancss = require('gulp-clean-css');
const uglify = require('gulp-uglify');
const gzip = require('gulp-gzip');
const inline = require('gulp-inline');
const inlineImages = require('gulp-css-base64');
const debug = require('gulp-debug');
const tap = require('gulp-tap');
const log = require('fancy-log');

const dataFolder = 'data/';
const outputFolder = 'data/';
 
function hex (filename, data) {
  var outputfile = outputFolder + filename + '.h';
  log('output file: ' + outputfile);
  
  var wstream = fs.createWriteStream(outputfile);
  wstream.on('error', function (err) {
        console.log(err);
    });

  var filevar = filename.replace(/\./g, '_');

  wstream.write('#define ' + filevar + '_len ' + data.length + '\n');
  wstream.write('const char ' + filevar +'[] PROGMEM = {');

  for (i=0; i<data.length; i++) {
    if (i % 1000 == 0) wstream.write("\n");
    wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
    if (i<data.length-1) wstream.write(',');
  }

  wstream.write('\n};')
  wstream.end();
  return data;
}


gulp.task('default', function() {
    return gulp.src(dataFolder + '*.html')
        .pipe(debug({title: 'file:'}))
        .pipe(inline({
            base: dataFolder,
            js: uglify,
            css: [cleancss, inlineImages],
            disabledTypes: ['svg', 'img']
        }))
        .pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        }))
        .pipe(gzip())
        .pipe(tap(function (file, t) {
          file.contents = Buffer.from(hex(file.relative, file.contents))
        }))
        // uncomment to write intermediate gzipped temp file
        //.pipe(gulp.dest(outputFolder));
});
