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
    `(log ,lvl ,data (port-filename (current-input-port)) (port-line-number (current-input-port))))

(define-macro (flog data lvl)
    `(log ,lvl ,data (funcinfo (*function*) 'file) (funcinfo (*function*) 'line)))

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

(define-macro (update . expr)
    (let ((f (gensym)))
        `(begin
	    (define (,f) (begin . ,expr))
	    (register 0 ,f))))

(define-macro (while condition . body)
    (let ((loop (gensym)))
    `(let ,loop ()
        (cond (,condition
	(begin . ,body)
	(,loop))))))

(define (clamp val min max)
    (cond ((< val min) min)
          ((> val max) max)
	  (else val)))

(define (lerp s f dt)
    (+ s (* (clamp dt 0 1) (- f s))))

(define (deg2rad deg)
    (* deg 0.01745329252))

(define (body_angle! body angle) (set_body body 0 angle))
(define (body_pos! body x y) (set_body_pos body x y))
