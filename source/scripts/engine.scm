(define-macro (time expr)
    `(let ((start (*s7* 'cpu-time)))
       (let ((res (list ,expr)))
         (list (car res)
       (- (*s7* 'cpu-time) start)))))

(define-macro (funcinfo func info)
    `(*function* (funclet ,func) ,info))

(define-macro (funcloc func)
    `(format #f "~A:~A" (funcinfo ,func 'file) (funcinfo ,func 'line)))

(define-macro (funcsrc func)
    `(funcinfo ,func 'source))

(define-macro (glog data lvl)
    (let ((f (gensym)))
        `(begin
	    (define (,f) ())
	    (log ,lvl ,data (funcinfo ,f 'file) (funcinfo ,f 'line)))))

(define (loginfo data) (glog data 0))
(define (logwarn data) (glog data 1))
(define (logerr data) (glog data 2))
(define (logcrit data) (glog data 3))
         

(define (set_fps fps) (settings_cmd 0 (/ 1 fps)))
(define (set_update fps) (settings_cmd 1 (/ 1 fps)))
(define (set_phys fps) (settings_cmd 2 (/ 1 fps)))

(define (win_fulltoggle w) (win_cmd w 0))
(define (win_fullscreen w) (win_cmd w 1))
(define (win_unfullscreen w) (win_cmd w 2))
(define (win_title s) (win_cmd 0 3 s))

(define (load_level s) (gen_cmd 0 s))
(define (load_prefab s) (gen_cmd 1 s))
(define (quit) (sys_cmd 0))
(define (exit) (quit))


(define (sound_play sound) (sound_cmd sound 0))
(define (sound_pause sound) (sound_cmd sound 1))
(define (sound_stop sound) (sound_cmd sound 2))
(define (sound_restart sound) (sound_cmd sound 3))
